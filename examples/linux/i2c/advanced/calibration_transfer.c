/*
Purpose: inspect calibration export chunks and optionally apply one import payload.
Defaults: /dev/i2c-1, address 99, and product pH.
Assumptions: the selected product supports calibration transfer and is already in I2C mode.
Next: read ../commissioning/readiness_check.c to inspect calibration state before import/export.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_calibration_transfer.h"
#include "ezo_product.h"

#include <stdio.h>
#include <string.h>

static int parse_product_id(const char *text, ezo_product_id_t *product_id_out) {
  if (text == NULL || product_id_out == NULL) {
    return 0;
  }
  if (strcmp(text, "ph") == 0) {
    *product_id_out = EZO_PRODUCT_PH;
    return 1;
  }
  if (strcmp(text, "orp") == 0) {
    *product_id_out = EZO_PRODUCT_ORP;
    return 1;
  }
  if (strcmp(text, "ec") == 0) {
    *product_id_out = EZO_PRODUCT_EC;
    return 1;
  }
  if (strcmp(text, "do") == 0) {
    *product_id_out = EZO_PRODUCT_DO;
    return 1;
  }
  if (strcmp(text, "rtd") == 0) {
    *product_id_out = EZO_PRODUCT_RTD;
    return 1;
  }
  if (strcmp(text, "hum") == 0) {
    *product_id_out = EZO_PRODUCT_HUM;
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_calibration_export_info_t export_info;
  ezo_calibration_import_result_t import_result;
  const ezo_product_metadata_t *metadata = NULL;
  const char *product_text = NULL;
  const char *payload = NULL;
  ezo_product_id_t product_id = EZO_PRODUCT_PH;
  ezo_timing_hint_t hint;
  char chunk[128];
  size_t chunk_len = 0;
  uint32_t chunk_index = 0;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 99U, &options, &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [address] [--product=ph|orp|ec|do|rtd|hum] "
            "[--payload=text] [--apply]\n",
            argv[0]);
    return 1;
  }

  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");
  product_text = ezo_example_find_option_value(argc, argv, next_arg, "--product=");
  payload = ezo_example_find_option_value(argc, argv, next_arg, "--payload=");

  if (product_text != NULL && !parse_product_id(product_text, &product_id)) {
    fprintf(stderr, "unsupported product: %s\n", product_text);
    return 1;
  }
  if (apply_requested && payload == NULL) {
    fprintf(stderr, "--apply requires --payload=<text>\n");
    return 1;
  }

  metadata = ezo_product_get_metadata(product_id);
  if (metadata == NULL ||
      !ezo_product_supports_capability(product_id, EZO_PRODUCT_CAP_CALIBRATION_TRANSFER)) {
    fprintf(stderr, "selected product does not support calibration transfer\n");
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_calibration_send_export_info_query_i2c(&session.device, product_id, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_calibration_read_export_info_i2c(&session.device, &export_info);
  }
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("export_info", result);
  }

  printf("transport=i2c\n");
  printf("product=%s\n", metadata->vendor_short_code);
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("export_chunk_count=%u\n", (unsigned)export_info.chunk_count);
  printf("export_byte_count=%u\n", (unsigned)export_info.byte_count);

  for (chunk_index = 0; chunk_index < export_info.chunk_count; ++chunk_index) {
    result = ezo_calibration_send_export_next_i2c(&session.device, product_id, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result =
          ezo_calibration_read_export_chunk_i2c(&session.device, chunk, sizeof(chunk), &chunk_len);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("export_chunk", result);
    }
    printf("export_chunk_%u=%.*s\n", (unsigned)chunk_index, (int)chunk_len, chunk);
  }

  printf("apply_requested=%d\n", apply_requested);
  printf("import_payload_length=%u\n", payload != NULL ? (unsigned)strlen(payload) : 0U);

  if (apply_requested) {
    result = ezo_calibration_send_import_i2c(&session.device, product_id, payload, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_calibration_read_import_result_i2c(&session.device, &import_result);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("import_payload", result);
    }

    printf("import_device_status_name=%s\n",
           ezo_device_status_name(import_result.device_status));
    printf("import_device_status_code=%u\n", (unsigned)import_result.device_status);
    printf("import_pending_reboot=%u\n", (unsigned)import_result.pending_reboot);
  }

  ezo_example_close_i2c(&session);
  return 0;
}
