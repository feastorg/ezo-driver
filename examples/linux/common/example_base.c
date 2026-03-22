#include "example_base.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ezo_example_is_flag(const char *text) {
  return text != NULL && text[0] == '-';
}

static int ezo_example_parse_uint32(const char *text, uint32_t *value_out) {
  char *end = NULL;
  unsigned long value = 0;

  if (text == NULL || value_out == NULL || text[0] == '\0') {
    return 0;
  }

  value = strtoul(text, &end, 10);
  if (end == NULL || *end != '\0') {
    return 0;
  }

  *value_out = (uint32_t)value;
  return 1;
}

static int ezo_example_parse_uint8(const char *text, uint8_t *value_out) {
  uint32_t value = 0;

  if (!ezo_example_parse_uint32(text, &value) || value > 127U) {
    return 0;
  }

  *value_out = (uint8_t)value;
  return 1;
}

static int ezo_example_parse_baud_value(const char *text,
                                        ezo_uart_posix_baud_t *baud_out,
                                        uint32_t *baud_rate_out) {
  uint32_t baud_rate = 0;

  if (!ezo_example_parse_uint32(text, &baud_rate)) {
    return 0;
  }

  switch (baud_rate) {
    case 300U:
      *baud_out = EZO_UART_POSIX_BAUD_300;
      break;
    case 1200U:
      *baud_out = EZO_UART_POSIX_BAUD_1200;
      break;
    case 2400U:
      *baud_out = EZO_UART_POSIX_BAUD_2400;
      break;
    case 9600U:
      *baud_out = EZO_UART_POSIX_BAUD_9600;
      break;
    case 19200U:
      *baud_out = EZO_UART_POSIX_BAUD_19200;
      break;
    case 38400U:
      *baud_out = EZO_UART_POSIX_BAUD_38400;
      break;
    case 57600U:
      *baud_out = EZO_UART_POSIX_BAUD_57600;
      break;
    case 115200U:
      *baud_out = EZO_UART_POSIX_BAUD_115200;
      break;
    default:
      return 0;
  }

  *baud_rate_out = baud_rate;
  return 1;
}

static int ezo_example_resolve_default_baud(uint32_t default_baud_rate,
                                            ezo_uart_posix_baud_t *baud_out,
                                            uint32_t *baud_rate_out) {
  switch (default_baud_rate) {
    case 300U:
      *baud_out = EZO_UART_POSIX_BAUD_300;
      break;
    case 1200U:
      *baud_out = EZO_UART_POSIX_BAUD_1200;
      break;
    case 2400U:
      *baud_out = EZO_UART_POSIX_BAUD_2400;
      break;
    case 9600U:
      *baud_out = EZO_UART_POSIX_BAUD_9600;
      break;
    case 19200U:
      *baud_out = EZO_UART_POSIX_BAUD_19200;
      break;
    case 38400U:
      *baud_out = EZO_UART_POSIX_BAUD_38400;
      break;
    case 57600U:
      *baud_out = EZO_UART_POSIX_BAUD_57600;
      break;
    case 115200U:
      *baud_out = EZO_UART_POSIX_BAUD_115200;
      break;
    default:
      return 0;
  }

  *baud_rate_out = default_baud_rate;
  return 1;
}

int ezo_example_parse_i2c_options(int argc,
                                  char **argv,
                                  uint8_t default_address,
                                  ezo_example_i2c_options_t *options_out,
                                  int *next_arg_out) {
  int index = 1;

  if (options_out == NULL || next_arg_out == NULL) {
    return 0;
  }

  options_out->device_path = EZO_EXAMPLE_I2C_DEFAULT_PATH;
  options_out->address = default_address;

  if (argc > index && !ezo_example_is_flag(argv[index])) {
    uint8_t parsed_address = 0;

    if (ezo_example_parse_uint8(argv[index], &parsed_address) &&
        (argc <= index + 1 || ezo_example_is_flag(argv[index + 1]))) {
      options_out->address = parsed_address;
      index += 1;
    } else {
      options_out->device_path = argv[index];
      index += 1;
    }
  }

  if (argc > index && !ezo_example_is_flag(argv[index])) {
    if (!ezo_example_parse_uint8(argv[index], &options_out->address)) {
      return 0;
    }
    index += 1;
  }

  *next_arg_out = index;
  return 1;
}

int ezo_example_parse_uart_options(int argc,
                                   char **argv,
                                   uint32_t default_baud_rate,
                                   ezo_example_uart_options_t *options_out,
                                   int *next_arg_out) {
  int index = 1;

  if (options_out == NULL || next_arg_out == NULL) {
    return 0;
  }

  options_out->device_path = EZO_EXAMPLE_UART_DEFAULT_PATH;
  if (!ezo_example_resolve_default_baud(default_baud_rate,
                                        &options_out->baud,
                                        &options_out->baud_rate)) {
    return 0;
  }

  if (argc > index && !ezo_example_is_flag(argv[index])) {
    ezo_uart_posix_baud_t parsed_baud = EZO_UART_POSIX_BAUD_9600;
    uint32_t parsed_baud_rate = 0;

    if (ezo_example_parse_baud_value(argv[index], &parsed_baud, &parsed_baud_rate) &&
        (argc <= index + 1 || ezo_example_is_flag(argv[index + 1]))) {
      options_out->baud = parsed_baud;
      options_out->baud_rate = parsed_baud_rate;
      index += 1;
    } else {
      options_out->device_path = argv[index];
      index += 1;
    }
  }

  if (argc > index && !ezo_example_is_flag(argv[index])) {
    if (!ezo_example_parse_baud_value(argv[index], &options_out->baud, &options_out->baud_rate)) {
      return 0;
    }
    index += 1;
  }

  *next_arg_out = index;
  return 1;
}

int ezo_example_has_flag(int argc, char **argv, int start_index, const char *flag) {
  int index = 0;

  if (argv == NULL || flag == NULL) {
    return 0;
  }

  for (index = start_index; index < argc; ++index) {
    if (strcmp(argv[index], flag) == 0) {
      return 1;
    }
  }

  return 0;
}

const char *ezo_example_find_option_value(int argc,
                                          char **argv,
                                          int start_index,
                                          const char *prefix) {
  size_t prefix_len = 0;
  int index = 0;

  if (argv == NULL || prefix == NULL) {
    return NULL;
  }

  prefix_len = strlen(prefix);
  for (index = start_index; index < argc; ++index) {
    if (strncmp(argv[index], prefix, prefix_len) == 0) {
      return argv[index] + prefix_len;
    }
  }

  return NULL;
}

void ezo_example_wait_hint(const ezo_timing_hint_t *hint) {
  if (hint == NULL || hint->wait_ms == 0U) {
    return;
  }

  usleep((useconds_t)(hint->wait_ms * 1000U));
}

int ezo_example_print_error(const char *step, ezo_result_t result) {
  fprintf(stderr, "step=%s result=%d\n", step != NULL ? step : "unknown", (int)result);
  return 1;
}

const char *ezo_example_bool_name(int value) {
  return value ? "enabled" : "disabled";
}

const char *ezo_example_product_name(ezo_product_id_t product_id) {
  const ezo_product_metadata_t *metadata = ezo_product_get_metadata(product_id);
  return metadata != NULL ? metadata->vendor_short_code : "unknown";
}

const char *ezo_example_transport_name(ezo_product_transport_t transport) {
  switch (transport) {
    case EZO_PRODUCT_TRANSPORT_UART:
      return "uart";
    case EZO_PRODUCT_TRANSPORT_I2C:
      return "i2c";
    default:
      return "unknown";
  }
}

const char *ezo_example_support_name(ezo_product_support_t support) {
  switch (support) {
    case EZO_PRODUCT_SUPPORT_METADATA:
      return "metadata";
    case EZO_PRODUCT_SUPPORT_TYPED_READ:
      return "typed_read";
    case EZO_PRODUCT_SUPPORT_FULL:
      return "full";
    default:
      return "unknown";
  }
}

const char *ezo_example_default_state_name(ezo_product_default_state_t state) {
  switch (state) {
    case EZO_PRODUCT_DEFAULT_DISABLED:
      return "disabled";
    case EZO_PRODUCT_DEFAULT_ENABLED:
      return "enabled";
    case EZO_PRODUCT_DEFAULT_QUERY_REQUIRED:
      return "query_required";
    default:
      return "unknown";
  }
}

const char *ezo_example_output_schema_name(ezo_product_output_schema_t schema) {
  switch (schema) {
    case EZO_PRODUCT_OUTPUT_SCHEMA_SCALAR_SINGLE:
      return "scalar_single";
    case EZO_PRODUCT_OUTPUT_SCHEMA_PRIMARY_ONLY:
      return "primary_only";
    case EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED:
      return "query_required";
    default:
      return "unknown";
  }
}

const char *ezo_example_uart_response_kind_name(
    ezo_uart_response_kind_t response_kind) {
  switch (response_kind) {
    case EZO_UART_RESPONSE_DATA:
      return "data";
    case EZO_UART_RESPONSE_OK:
      return "ok";
    case EZO_UART_RESPONSE_ERROR:
      return "error";
    case EZO_UART_RESPONSE_OVER_VOLTAGE:
      return "over_voltage";
    case EZO_UART_RESPONSE_UNDER_VOLTAGE:
      return "under_voltage";
    case EZO_UART_RESPONSE_RESET:
      return "reset";
    case EZO_UART_RESPONSE_READY:
      return "ready";
    case EZO_UART_RESPONSE_SLEEP:
      return "sleep";
    case EZO_UART_RESPONSE_WAKE:
      return "wake";
    case EZO_UART_RESPONSE_DONE:
      return "done";
    default:
      return "unknown";
  }
}

void ezo_example_print_device_info(const ezo_device_info_t *info) {
  if (info == NULL) {
    return;
  }

  printf("product_id=%d\n", (int)info->product_id);
  printf("product_code=%s\n", info->product_code);
  printf("firmware_version=%s\n", info->firmware_version);
}

void ezo_example_print_product_metadata(const ezo_product_metadata_t *metadata) {
  if (metadata == NULL) {
    printf("metadata_known=0\n");
    return;
  }

  printf("metadata_known=1\n");
  printf("family_name=%s\n", metadata->family_name);
  printf("vendor_short_code=%s\n", metadata->vendor_short_code);
  printf("support_tier=%s\n", ezo_example_support_name(metadata->support_tier));
  printf("default_transport=%s\n", ezo_example_transport_name(metadata->default_transport));
  printf("default_i2c_address=%u\n", (unsigned)metadata->default_i2c_address);
  printf("default_continuous_mode=%s\n",
         ezo_example_default_state_name(metadata->default_continuous_mode));
  printf("default_response_codes=%s\n",
         ezo_example_default_state_name(metadata->default_response_codes));
  printf("default_output_schema=%s\n",
         ezo_example_output_schema_name(metadata->default_output_schema));
  printf("default_output_count=%u\n", (unsigned)metadata->default_output_count);
  printf("capability_flags=%u\n", (unsigned)metadata->capability_flags);
  printf("command_family_flags=%u\n", (unsigned)metadata->command_family_flags);
  printf("uart_timing_generic_ms=%u\n", (unsigned)metadata->uart_timing.generic_ms);
  printf("uart_timing_read_ms=%u\n", (unsigned)metadata->uart_timing.read_ms);
  printf("uart_timing_read_with_temp_comp_ms=%u\n",
         (unsigned)metadata->uart_timing.read_with_temp_comp_ms);
  printf("uart_timing_calibration_ms=%u\n", (unsigned)metadata->uart_timing.calibration_ms);
  printf("i2c_timing_generic_ms=%u\n", (unsigned)metadata->i2c_timing.generic_ms);
  printf("i2c_timing_read_ms=%u\n", (unsigned)metadata->i2c_timing.read_ms);
  printf("i2c_timing_read_with_temp_comp_ms=%u\n",
         (unsigned)metadata->i2c_timing.read_with_temp_comp_ms);
  printf("i2c_timing_calibration_ms=%u\n", (unsigned)metadata->i2c_timing.calibration_ms);
}
