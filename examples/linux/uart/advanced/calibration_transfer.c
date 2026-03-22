/*
Purpose: inspect calibration export chunks and optionally apply one import payload.
Defaults: /dev/ttyUSB0 at 9600 baud and product pH.
Assumptions: the selected product supports calibration transfer and response-code mode can be bootstrapped.
Next: read ../commissioning/readiness_check.c to inspect calibration state before import/export.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_calibration_transfer.h"

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
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_calibration_export_info_t export_info;
  const ezo_product_metadata_t *metadata = NULL;
  const char *product_text = NULL;
  const char *payload = NULL;
  ezo_product_id_t product_id = EZO_PRODUCT_PH;
  ezo_timing_hint_t hint;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char chunk[128];
  size_t chunk_len = 0;
  uint32_t chunk_index = 0;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;

  if (!ezo_example_parse_uart_options(argc,
                                      argv,
                                      EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE,
                                      &options,
                                      &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [baud] [--product=ph|orp|ec|do|rtd|hum] "
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

  result = ezo_example_open_uart(options.device_path, options.baud, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_uart", result);
  }

  result =
      ezo_example_uart_bootstrap_response_codes(&session.device, product_id, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_calibration_send_export_info_query_uart(&session.device, product_id, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_calibration_read_export_info_uart(&session.device, &export_info);
  }
  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("export_info", result);
  }

  printf("transport=uart\n");
  printf("product=%s\n", ezo_example_product_name(product_id));
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("export_chunk_count=%u\n", (unsigned)export_info.chunk_count);
  printf("export_byte_count=%u\n", (unsigned)export_info.byte_count);

  for (chunk_index = 0; chunk_index < export_info.chunk_count; ++chunk_index) {
    int saw_data = 0;

    result = ezo_calibration_send_export_next_uart(&session.device, product_id, &hint);
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("send_export_next", result);
    }
    ezo_example_wait_hint(&hint);

    for (;;) {
      result = ezo_calibration_read_export_chunk_uart(&session.device,
                                                      chunk,
                                                      sizeof(chunk),
                                                      &chunk_len,
                                                      &kind);
      if (result != EZO_OK) {
        ezo_example_close_uart(&session);
        return ezo_example_print_error("read_export_chunk", result);
      }

      if (kind == EZO_UART_RESPONSE_DATA) {
        printf("export_chunk_%u=%.*s\n", (unsigned)chunk_index, (int)chunk_len, chunk);
        saw_data = 1;
        continue;
      }

      if (kind == EZO_UART_RESPONSE_DONE || kind == EZO_UART_RESPONSE_OK) {
        break;
      }

      ezo_example_close_uart(&session);
      return ezo_example_print_error("unexpected_export_terminal", EZO_ERR_PROTOCOL);
    }

    if (!saw_data) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("missing_export_chunk", EZO_ERR_PROTOCOL);
    }
  }

  printf("apply_requested=%d\n", apply_requested);
  printf("import_payload_length=%u\n", payload != NULL ? (unsigned)strlen(payload) : 0U);

  if (apply_requested) {
    result = ezo_calibration_send_import_uart(&session.device, product_id, payload, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_calibration_read_import_result_uart(&session.device, &kind);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("import_payload", result);
    }

    printf("import_response_kind=%s\n", ezo_example_uart_response_kind_name(kind));
  }

  ezo_example_close_uart(&session);
  return 0;
}
