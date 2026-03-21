#include "ezo_do.h"

#include "ezo_common.h"
#include "ezo_parse.h"
#include "ezo_product.h"
#include "ezo_schema.h"

#include <string.h>

enum {
  EZO_DO_RESPONSE_BUFFER_LEN = 64
};

static ezo_result_t ezo_do_copy_command(char *buffer, size_t buffer_len, const char *command) {
  size_t command_len = 0;

  if (buffer == NULL || buffer_len == 0 || command == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  command_len = strlen(command);
  if (command_len + 1U > buffer_len) {
    return EZO_ERR_BUFFER_TOO_SMALL;
  }

  memcpy(buffer, command, command_len + 1U);
  return EZO_OK;
}

static ezo_result_t ezo_do_send_i2c_command(ezo_i2c_device_t *device,
                                            const char *command,
                                            ezo_command_kind_t kind,
                                            ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_DO,
                                                        EZO_PRODUCT_TRANSPORT_I2C,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_do_send_uart_command(ezo_uart_device_t *device,
                                             const char *command,
                                             ezo_command_kind_t kind,
                                             ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_DO,
                                                        EZO_PRODUCT_TRANSPORT_UART,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_do_send_i2c_float_command(ezo_i2c_device_t *device,
                                                  const char *prefix,
                                                  double value,
                                                  uint8_t decimals,
                                                  ezo_command_kind_t kind,
                                                  ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_DO,
                                                        EZO_PRODUCT_TRANSPORT_I2C,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command_with_float(device, prefix, value, decimals, kind, NULL);
}

static ezo_result_t ezo_do_send_uart_float_command(ezo_uart_device_t *device,
                                                   const char *prefix,
                                                   double value,
                                                   uint8_t decimals,
                                                   ezo_command_kind_t kind,
                                                   ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_DO,
                                                        EZO_PRODUCT_TRANSPORT_UART,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command_with_float(device, prefix, value, decimals, kind, NULL);
}

static ezo_result_t ezo_do_read_i2c_text(ezo_i2c_device_t *device,
                                         char *buffer,
                                         size_t buffer_len,
                                         size_t *response_len) {
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  ezo_result_t result = ezo_read_response(device, buffer, buffer_len, response_len, &status);
  if (result != EZO_OK) {
    return result;
  }

  if (status != EZO_STATUS_SUCCESS) {
    return EZO_ERR_PROTOCOL;
  }

  return EZO_OK;
}

static ezo_result_t ezo_do_read_uart_line_of_kind(ezo_uart_device_t *device,
                                                  ezo_uart_response_kind_t expected_kind,
                                                  char *buffer,
                                                  size_t buffer_len,
                                                  size_t *response_len) {
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  ezo_result_t result =
      ezo_uart_read_line(device, buffer, buffer_len, response_len, &kind);
  if (result != EZO_OK) {
    return result;
  }

  if (kind != expected_kind) {
    return EZO_ERR_PROTOCOL;
  }

  return EZO_OK;
}

static ezo_result_t ezo_do_read_uart_data_line(ezo_uart_device_t *device,
                                               char *buffer,
                                               size_t buffer_len,
                                               size_t *response_len) {
  return ezo_do_read_uart_line_of_kind(device,
                                       EZO_UART_RESPONSE_DATA,
                                       buffer,
                                       buffer_len,
                                       response_len);
}

static ezo_result_t ezo_do_read_uart_data_then_ok(ezo_uart_device_t *device,
                                                  char *buffer,
                                                  size_t buffer_len,
                                                  size_t *response_len) {
  char status_buffer[8];
  size_t status_len = 0;
  ezo_result_t result = ezo_do_read_uart_data_line(device, buffer, buffer_len, response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_read_uart_line_of_kind(device,
                                       EZO_UART_RESPONSE_OK,
                                       status_buffer,
                                       sizeof(status_buffer),
                                       &status_len);
}

static ezo_result_t ezo_do_parse_query_double(const char *buffer,
                                              size_t buffer_len,
                                              const char *prefix,
                                              double *value_out) {
  ezo_text_span_t fields[2];
  size_t field_count = 0;
  ezo_result_t result =
      ezo_parse_prefixed_fields(buffer, buffer_len, prefix, fields, 2, &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (field_count < 1) {
    return EZO_ERR_PARSE;
  }

  return ezo_parse_text_span_double(fields[0], value_out);
}

static int ezo_do_span_is_ascii_ci(ezo_text_span_t span, const char *text) {
  size_t i = 0;

  if (text == NULL || span.text == NULL) {
    return 0;
  }

  for (i = 0; text[i] != '\0'; ++i) {
    char left = '\0';
    char right = text[i];

    if (i >= span.length) {
      return 0;
    }

    left = span.text[i];
    if (left >= 'A' && left <= 'Z') {
      left = (char)(left - 'A' + 'a');
    }
    if (right >= 'A' && right <= 'Z') {
      right = (char)(right - 'A' + 'a');
    }

    if (left != right) {
      return 0;
    }
  }

  return span.length == i;
}

static int ezo_do_span_is_microsiemens(ezo_text_span_t span) {
  if (span.text == NULL) {
    return 0;
  }

  if (span.length == 2 &&
      (span.text[0] == 'u' || span.text[0] == 'U') &&
      (span.text[1] == 's' || span.text[1] == 'S')) {
    return 1;
  }

  if (span.length == 3 &&
      (unsigned char)span.text[0] == 0xC2 &&
      (unsigned char)span.text[1] == 0xB5 &&
      (span.text[2] == 's' || span.text[2] == 'S')) {
    return 1;
  }

  return 0;
}

static ezo_result_t ezo_do_parse_output_field(ezo_text_span_t field,
                                              ezo_do_output_mask_t *mask_out) {
  if (mask_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (ezo_text_span_equals_cstr(field, "mg")) {
    *mask_out = EZO_DO_OUTPUT_MG_L;
    return EZO_OK;
  }

  if (ezo_text_span_equals_cstr(field, "%")) {
    *mask_out = EZO_DO_OUTPUT_PERCENT_SATURATION;
    return EZO_OK;
  }

  return EZO_ERR_PARSE;
}

ezo_result_t ezo_do_parse_reading(const char *buffer,
                                  size_t buffer_len,
                                  ezo_do_output_mask_t enabled_mask,
                                  ezo_do_reading_t *reading_out) {
  ezo_output_schema_t schema;
  ezo_multi_output_reading_t reading;
  ezo_result_t result = EZO_OK;

  if (reading_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_schema_get_output_schema(EZO_PRODUCT_DO, &schema);
  if (result != EZO_OK) {
    return result;
  }

  result = ezo_schema_parse_multi_output_reading(buffer,
                                                 buffer_len,
                                                 &schema,
                                                 enabled_mask,
                                                 &reading);
  if (result != EZO_OK) {
    return result;
  }

  reading_out->present_mask = reading.present_mask;
  reading_out->milligrams_per_liter = reading.values[0].value;
  reading_out->percent_saturation = reading.values[1].value;
  return EZO_OK;
}

ezo_result_t ezo_do_parse_output_config(const char *buffer,
                                        size_t buffer_len,
                                        ezo_do_output_config_t *config_out) {
  ezo_text_span_t prefix;
  ezo_text_span_t fields[EZO_SCHEMA_MAX_FIELDS];
  size_t field_count = 0;
  size_t i = 0;
  ezo_result_t result = EZO_OK;
  ezo_do_output_mask_t enabled_mask = 0;

  if (config_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_query_response(buffer,
                                    buffer_len,
                                    &prefix,
                                    fields,
                                    EZO_SCHEMA_MAX_FIELDS,
                                    &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (!ezo_text_span_equals_cstr(prefix, "?O")) {
    return EZO_ERR_PARSE;
  }

  for (i = 0; i < field_count; ++i) {
    ezo_do_output_mask_t output = 0;
    result = ezo_do_parse_output_field(fields[i], &output);
    if (result != EZO_OK) {
      return result;
    }
    enabled_mask |= output;
  }

  config_out->enabled_mask = enabled_mask;
  return EZO_OK;
}

ezo_result_t ezo_do_parse_temperature(const char *buffer,
                                      size_t buffer_len,
                                      ezo_do_temperature_compensation_t *temperature_out) {
  if (temperature_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return ezo_do_parse_query_double(buffer,
                                   buffer_len,
                                   "?T",
                                   &temperature_out->temperature_c);
}

ezo_result_t ezo_do_parse_salinity(const char *buffer,
                                   size_t buffer_len,
                                   ezo_do_salinity_compensation_t *salinity_out) {
  ezo_text_span_t fields[2];
  size_t field_count = 0;
  ezo_result_t result = EZO_OK;

  if (salinity_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_prefixed_fields(buffer, buffer_len, "?S", fields, 2, &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (field_count == 0 || field_count > 2) {
    return EZO_ERR_PARSE;
  }

  result = ezo_parse_text_span_double(fields[0], &salinity_out->value);
  if (result != EZO_OK) {
    return result;
  }

  salinity_out->unit = EZO_DO_SALINITY_UNIT_MICROSIEMENS;
  if (field_count == 2) {
    if (ezo_do_span_is_ascii_ci(fields[1], "ppt")) {
      salinity_out->unit = EZO_DO_SALINITY_UNIT_PPT;
    } else if (!ezo_do_span_is_microsiemens(fields[1])) {
      return EZO_ERR_PARSE;
    }
  }

  return EZO_OK;
}

ezo_result_t ezo_do_parse_pressure(const char *buffer,
                                   size_t buffer_len,
                                   ezo_do_pressure_compensation_t *pressure_out) {
  if (pressure_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return ezo_do_parse_query_double(buffer, buffer_len, "?P", &pressure_out->pressure_kpa);
}

ezo_result_t ezo_do_build_output_command(char *buffer,
                                         size_t buffer_len,
                                         ezo_do_output_mask_t output,
                                         uint8_t enabled) {
  switch (output) {
  case EZO_DO_OUTPUT_MG_L:
    return ezo_do_copy_command(buffer, buffer_len, enabled != 0 ? "O,mg,1" : "O,mg,0");
  case EZO_DO_OUTPUT_PERCENT_SATURATION:
    return ezo_do_copy_command(buffer, buffer_len, enabled != 0 ? "O,%,1" : "O,%,0");
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }
}

ezo_result_t ezo_do_build_temperature_command(char *buffer,
                                              size_t buffer_len,
                                              double temperature_c,
                                              uint8_t decimals) {
  return ezo_common_format_fixed_command(buffer, buffer_len, "T,", temperature_c, decimals);
}

ezo_result_t ezo_do_build_salinity_command(char *buffer,
                                           size_t buffer_len,
                                           double value,
                                           ezo_do_salinity_unit_t unit,
                                           uint8_t decimals) {
  static const char salinity_microsiemens_suffix[] = ",uS";
  static const char salinity_ppt_suffix[] = ",ppt";
  const char *suffix = NULL;
  size_t suffix_len = 0;
  ezo_result_t result = EZO_OK;

  if (buffer == NULL || buffer_len == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_common_format_fixed_command(buffer, buffer_len, "S,", value, decimals);
  if (result != EZO_OK) {
    return result;
  }

  switch (unit) {
  case EZO_DO_SALINITY_UNIT_MICROSIEMENS:
    suffix = salinity_microsiemens_suffix;
    break;
  case EZO_DO_SALINITY_UNIT_PPT:
    suffix = salinity_ppt_suffix;
    break;
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }

  suffix_len = strlen(suffix);
  if (strlen(buffer) + suffix_len + 1U > buffer_len) {
    return EZO_ERR_BUFFER_TOO_SMALL;
  }

  memcpy(buffer + strlen(buffer), suffix, suffix_len + 1U);
  return EZO_OK;
}

ezo_result_t ezo_do_build_pressure_command(char *buffer,
                                           size_t buffer_len,
                                           double pressure_kpa,
                                           uint8_t decimals) {
  return ezo_common_format_fixed_command(buffer, buffer_len, "P,", pressure_kpa, decimals);
}

ezo_result_t ezo_do_send_read_i2c(ezo_i2c_device_t *device,
                                  ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_i2c_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_do_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device,
                                                 double temperature_c,
                                                 uint8_t decimals,
                                                 ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_i2c_float_command(device,
                                       "rt,",
                                       temperature_c,
                                       decimals,
                                       EZO_COMMAND_READ_WITH_TEMP_COMP,
                                       timing_hint);
}

ezo_result_t ezo_do_send_output_query_i2c(ezo_i2c_device_t *device,
                                          ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_i2c_command(device, "O,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_output_set_i2c(ezo_i2c_device_t *device,
                                        ezo_do_output_mask_t output,
                                        uint8_t enabled,
                                        ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result = ezo_do_build_output_command(command, sizeof(command), output, enabled);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_temperature_query_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_i2c_command(device, "T,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_temperature_set_i2c(ezo_i2c_device_t *device,
                                             double temperature_c,
                                             uint8_t decimals,
                                             ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_do_build_temperature_command(command, sizeof(command), temperature_c, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_salinity_query_i2c(ezo_i2c_device_t *device,
                                            ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_i2c_command(device, "S,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_salinity_set_i2c(ezo_i2c_device_t *device,
                                          double value,
                                          ezo_do_salinity_unit_t unit,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_do_build_salinity_command(command, sizeof(command), value, unit, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_pressure_query_i2c(ezo_i2c_device_t *device,
                                            ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_i2c_command(device, "P,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_pressure_set_i2c(ezo_i2c_device_t *device,
                                          double pressure_kpa,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_do_build_pressure_command(command, sizeof(command), pressure_kpa, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_read_response_i2c(ezo_i2c_device_t *device,
                                      ezo_do_output_mask_t enabled_mask,
                                      ezo_do_reading_t *reading_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_do_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_reading(buffer, response_len, enabled_mask, reading_out);
}

ezo_result_t ezo_do_read_output_config_i2c(ezo_i2c_device_t *device,
                                           ezo_do_output_config_t *config_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_do_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_output_config(buffer, response_len, config_out);
}

ezo_result_t ezo_do_read_temperature_i2c(ezo_i2c_device_t *device,
                                         ezo_do_temperature_compensation_t *temperature_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_do_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_temperature(buffer, response_len, temperature_out);
}

ezo_result_t ezo_do_read_salinity_i2c(ezo_i2c_device_t *device,
                                      ezo_do_salinity_compensation_t *salinity_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_do_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_salinity(buffer, response_len, salinity_out);
}

ezo_result_t ezo_do_read_pressure_i2c(ezo_i2c_device_t *device,
                                      ezo_do_pressure_compensation_t *pressure_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_do_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_pressure(buffer, response_len, pressure_out);
}

ezo_result_t ezo_do_send_read_uart(ezo_uart_device_t *device,
                                   ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_uart_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_do_send_read_with_temp_comp_uart(ezo_uart_device_t *device,
                                                  double temperature_c,
                                                  uint8_t decimals,
                                                  ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_uart_float_command(device,
                                        "rt,",
                                        temperature_c,
                                        decimals,
                                        EZO_COMMAND_READ_WITH_TEMP_COMP,
                                        timing_hint);
}

ezo_result_t ezo_do_send_output_query_uart(ezo_uart_device_t *device,
                                           ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_uart_command(device, "O,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_output_set_uart(ezo_uart_device_t *device,
                                         ezo_do_output_mask_t output,
                                         uint8_t enabled,
                                         ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result = ezo_do_build_output_command(command, sizeof(command), output, enabled);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_temperature_query_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_uart_command(device, "T,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_temperature_set_uart(ezo_uart_device_t *device,
                                              double temperature_c,
                                              uint8_t decimals,
                                              ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_do_build_temperature_command(command, sizeof(command), temperature_c, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_salinity_query_uart(ezo_uart_device_t *device,
                                             ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_uart_command(device, "S,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_salinity_set_uart(ezo_uart_device_t *device,
                                           double value,
                                           ezo_do_salinity_unit_t unit,
                                           uint8_t decimals,
                                           ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_do_build_salinity_command(command, sizeof(command), value, unit, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_pressure_query_uart(ezo_uart_device_t *device,
                                             ezo_timing_hint_t *timing_hint) {
  return ezo_do_send_uart_command(device, "P,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_send_pressure_set_uart(ezo_uart_device_t *device,
                                           double pressure_kpa,
                                           uint8_t decimals,
                                           ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_do_build_pressure_command(command, sizeof(command), pressure_kpa, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_do_read_response_uart(ezo_uart_device_t *device,
                                       ezo_do_output_mask_t enabled_mask,
                                       ezo_do_reading_t *reading_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_do_read_uart_data_line(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_reading(buffer, response_len, enabled_mask, reading_out);
}

ezo_result_t ezo_do_read_output_config_uart(ezo_uart_device_t *device,
                                            ezo_do_output_config_t *config_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_do_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_output_config(buffer, response_len, config_out);
}

ezo_result_t ezo_do_read_temperature_uart(
    ezo_uart_device_t *device,
    ezo_do_temperature_compensation_t *temperature_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_do_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_temperature(buffer, response_len, temperature_out);
}

ezo_result_t ezo_do_read_salinity_uart(ezo_uart_device_t *device,
                                       ezo_do_salinity_compensation_t *salinity_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_do_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_salinity(buffer, response_len, salinity_out);
}

ezo_result_t ezo_do_read_pressure_uart(ezo_uart_device_t *device,
                                       ezo_do_pressure_compensation_t *pressure_out) {
  char buffer[EZO_DO_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_do_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_do_parse_pressure(buffer, response_len, pressure_out);
}
