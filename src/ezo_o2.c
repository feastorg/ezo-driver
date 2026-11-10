#include "ezo_o2.h"

#include "ezo_common.h"
#include "ezo_parse.h"
#include "ezo_product.h"
#include "ezo_schema.h"

#include <string.h>

enum {
  EZO_O2_RESPONSE_BUFFER_LEN = 64
};

static ezo_result_t ezo_o2_copy_command(char *buffer, size_t buffer_len, const char *command) {
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

static ezo_result_t ezo_o2_resolve_hint(ezo_product_transport_t transport,
                                        ezo_command_kind_t kind,
                                        ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  return ezo_product_resolve_timing_hint(EZO_PRODUCT_O2,
                                         transport,
                                         kind,
                                         timing_hint != NULL ? timing_hint : &local_hint);
}

static ezo_result_t ezo_o2_send_i2c_command(ezo_i2c_device_t *device,
                                            const char *command,
                                            ezo_command_kind_t kind,
                                            ezo_timing_hint_t *timing_hint) {
  ezo_result_t result = ezo_o2_resolve_hint(EZO_PRODUCT_TRANSPORT_I2C, kind, timing_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_o2_send_uart_command(ezo_uart_device_t *device,
                                             const char *command,
                                             ezo_command_kind_t kind,
                                             ezo_timing_hint_t *timing_hint) {
  ezo_result_t result = ezo_o2_resolve_hint(EZO_PRODUCT_TRANSPORT_UART, kind, timing_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_o2_read_i2c_text(ezo_i2c_device_t *device,
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

static ezo_result_t ezo_o2_read_uart_data_line(ezo_uart_device_t *device,
                                               char *buffer,
                                               size_t buffer_len,
                                               size_t *response_len) {
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  ezo_result_t result = ezo_uart_read_line(device, buffer, buffer_len, response_len, &kind);
  if (result != EZO_OK) {
    return result;
  }

  if (kind != EZO_UART_RESPONSE_DATA) {
    return EZO_ERR_PROTOCOL;
  }

  return EZO_OK;
}

static ezo_result_t ezo_o2_read_uart_data_then_ok(ezo_uart_device_t *device,
                                                  char *buffer,
                                                  size_t buffer_len,
                                                  size_t *response_len) {
  ezo_result_t result = ezo_o2_read_uart_data_line(device, buffer, buffer_len, response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_read_ok(device);
}

static ezo_result_t ezo_o2_read_uart_ok_then_data(ezo_uart_device_t *device,
                                                  char *buffer,
                                                  size_t buffer_len,
                                                  size_t *response_len) {
  ezo_result_t result = ezo_uart_read_ok(device);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_read_uart_data_line(device, buffer, buffer_len, response_len);
}

static int ezo_o2_output_query_prefix_is_valid(ezo_text_span_t prefix,
                                               const ezo_text_span_t *fields,
                                               size_t field_count,
                                               size_t *field_offset_out) {
  if (ezo_text_span_equals_cstr(prefix, "?O")) {
    *field_offset_out = 0;
    return 1;
  }

  if (ezo_text_span_equals_cstr(prefix, "?") && field_count > 0 &&
      ezo_text_span_equals_cstr(fields[0], "O")) {
    *field_offset_out = 1;
    return 1;
  }

  return 0;
}

static ezo_result_t ezo_o2_parse_output_field(ezo_text_span_t field,
                                              ezo_o2_output_mask_t *mask_out) {
  if (ezo_text_span_equals_cstr(field, "%")) {
    *mask_out = EZO_O2_OUTPUT_PERCENT;
    return EZO_OK;
  }

  if (ezo_text_span_equals_cstr(field, "PPT")) {
    *mask_out = EZO_O2_OUTPUT_PPT;
    return EZO_OK;
  }

  return EZO_ERR_PARSE;
}

ezo_result_t ezo_o2_parse_reading(const char *buffer,
                                  size_t buffer_len,
                                  ezo_o2_output_mask_t enabled_mask,
                                  ezo_o2_reading_t *reading_out) {
  ezo_output_schema_t schema;
  ezo_multi_output_reading_t reading;
  ezo_result_t result = EZO_OK;

  if (reading_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_schema_get_output_schema(EZO_PRODUCT_O2, &schema);
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
  reading_out->percent = reading.values[0].value;
  reading_out->parts_per_thousand = reading.values[1].value;
  return EZO_OK;
}

ezo_result_t ezo_o2_parse_output_config(const char *buffer,
                                        size_t buffer_len,
                                        ezo_o2_output_config_t *config_out) {
  ezo_text_span_t prefix;
  ezo_text_span_t fields[EZO_SCHEMA_MAX_FIELDS];
  size_t field_count = 0;
  size_t field_offset = 0;
  size_t i = 0;
  ezo_result_t result = EZO_OK;
  ezo_o2_output_mask_t enabled_mask = 0;

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

  if (!ezo_o2_output_query_prefix_is_valid(prefix, fields, field_count, &field_offset)) {
    return EZO_ERR_PARSE;
  }

  for (i = field_offset; i < field_count; ++i) {
    ezo_o2_output_mask_t output = 0;
    result = ezo_o2_parse_output_field(fields[i], &output);
    if (result != EZO_OK) {
      return result;
    }
    enabled_mask |= output;
  }

  config_out->enabled_mask = enabled_mask;
  return EZO_OK;
}

ezo_result_t ezo_o2_parse_temperature(const char *buffer,
                                      size_t buffer_len,
                                      ezo_o2_temperature_compensation_t *temperature_out) {
  ezo_text_span_t fields[1];
  size_t field_count = 0;
  ezo_result_t result = EZO_OK;

  if (temperature_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_prefixed_fields(buffer, buffer_len, "?T", fields, 1, &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (field_count != 1) {
    return EZO_ERR_PARSE;
  }

  return ezo_parse_text_span_double(fields[0], &temperature_out->temperature_c);
}

ezo_result_t ezo_o2_build_output_command(char *buffer,
                                         size_t buffer_len,
                                         ezo_o2_output_mask_t output,
                                         uint8_t enabled) {
  switch (output) {
  case EZO_O2_OUTPUT_PERCENT:
    return ezo_o2_copy_command(buffer, buffer_len, enabled != 0 ? "O,%,1" : "O,%,0");
  case EZO_O2_OUTPUT_PPT:
    return ezo_o2_copy_command(buffer, buffer_len, enabled != 0 ? "O,PPT,1" : "O,PPT,0");
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }
}

ezo_result_t ezo_o2_build_temperature_command(char *buffer,
                                              size_t buffer_len,
                                              double temperature_c,
                                              uint8_t decimals) {
  return ezo_common_format_fixed_command(buffer, buffer_len, "T,", temperature_c, decimals);
}

ezo_result_t ezo_o2_send_read_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint) {
  return ezo_o2_send_i2c_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_o2_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device,
                                                 double temperature_c,
                                                 uint8_t decimals,
                                                 ezo_timing_hint_t *timing_hint) {
  ezo_result_t result = ezo_o2_resolve_hint(EZO_PRODUCT_TRANSPORT_I2C,
                                            EZO_COMMAND_READ_WITH_TEMP_COMP,
                                            timing_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command_with_float(device,
                                     "rt,",
                                     temperature_c,
                                     decimals,
                                     EZO_COMMAND_READ_WITH_TEMP_COMP,
                                     NULL);
}

ezo_result_t ezo_o2_send_output_query_i2c(ezo_i2c_device_t *device,
                                          ezo_timing_hint_t *timing_hint) {
  return ezo_o2_send_i2c_command(device, "O,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_o2_send_output_set_i2c(ezo_i2c_device_t *device,
                                        ezo_o2_output_mask_t output,
                                        uint8_t enabled,
                                        ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result = ezo_o2_build_output_command(command, sizeof(command), output, enabled);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_o2_send_temperature_query_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_o2_send_i2c_command(device, "T,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_o2_send_temperature_set_i2c(ezo_i2c_device_t *device,
                                             double temperature_c,
                                             uint8_t decimals,
                                             ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_o2_build_temperature_command(command, sizeof(command), temperature_c, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_o2_read_response_i2c(ezo_i2c_device_t *device,
                                      ezo_o2_output_mask_t enabled_mask,
                                      ezo_o2_reading_t *reading_out) {
  char buffer[EZO_O2_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_o2_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_parse_reading(buffer, response_len, enabled_mask, reading_out);
}

ezo_result_t ezo_o2_read_output_config_i2c(ezo_i2c_device_t *device,
                                           ezo_o2_output_config_t *config_out) {
  char buffer[EZO_O2_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_o2_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_parse_output_config(buffer, response_len, config_out);
}

ezo_result_t ezo_o2_read_temperature_i2c(ezo_i2c_device_t *device,
                                         ezo_o2_temperature_compensation_t *temperature_out) {
  char buffer[EZO_O2_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_o2_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_parse_temperature(buffer, response_len, temperature_out);
}

ezo_result_t ezo_o2_send_read_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint) {
  return ezo_o2_send_uart_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_o2_send_read_with_temp_comp_uart(ezo_uart_device_t *device,
                                                  double temperature_c,
                                                  uint8_t decimals,
                                                  ezo_timing_hint_t *timing_hint) {
  ezo_result_t result = ezo_o2_resolve_hint(EZO_PRODUCT_TRANSPORT_UART,
                                            EZO_COMMAND_READ_WITH_TEMP_COMP,
                                            timing_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command_with_float(device,
                                          "rt,",
                                          temperature_c,
                                          decimals,
                                          EZO_COMMAND_READ_WITH_TEMP_COMP,
                                          NULL);
}

ezo_result_t ezo_o2_send_output_query_uart(ezo_uart_device_t *device,
                                           ezo_timing_hint_t *timing_hint) {
  return ezo_o2_send_uart_command(device, "O,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_o2_send_output_set_uart(ezo_uart_device_t *device,
                                         ezo_o2_output_mask_t output,
                                         uint8_t enabled,
                                         ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result = ezo_o2_build_output_command(command, sizeof(command), output, enabled);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_o2_send_temperature_query_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_o2_send_uart_command(device, "T,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_o2_send_temperature_set_uart(ezo_uart_device_t *device,
                                              double temperature_c,
                                              uint8_t decimals,
                                              ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_o2_build_temperature_command(command, sizeof(command), temperature_c, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_o2_read_response_uart(ezo_uart_device_t *device,
                                       ezo_o2_output_mask_t enabled_mask,
                                       ezo_o2_reading_t *reading_out) {
  char buffer[EZO_O2_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_o2_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_parse_reading(buffer, response_len, enabled_mask, reading_out);
}

ezo_result_t ezo_o2_read_response_with_temp_comp_uart(ezo_uart_device_t *device,
                                                      ezo_o2_output_mask_t enabled_mask,
                                                      ezo_o2_reading_t *reading_out) {
  char buffer[EZO_O2_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_o2_read_uart_ok_then_data(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_parse_reading(buffer, response_len, enabled_mask, reading_out);
}

ezo_result_t ezo_o2_read_output_config_uart(ezo_uart_device_t *device,
                                            ezo_o2_output_config_t *config_out) {
  char buffer[EZO_O2_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_o2_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_parse_output_config(buffer, response_len, config_out);
}

ezo_result_t ezo_o2_read_temperature_uart(ezo_uart_device_t *device,
                                          ezo_o2_temperature_compensation_t *temperature_out) {
  char buffer[EZO_O2_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_o2_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_o2_parse_temperature(buffer, response_len, temperature_out);
}
