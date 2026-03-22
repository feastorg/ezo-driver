#include "ezo_rtd.h"

#include "ezo_common.h"
#include "ezo_parse.h"
#include "ezo_product.h"
#include "ezo_schema.h"

#include <string.h>

enum {
  EZO_RTD_RESPONSE_BUFFER_LEN = EZO_UART_MAX_TEXT_RESPONSE_CAPACITY
};

static ezo_result_t ezo_rtd_copy_command(char *buffer, size_t buffer_len, const char *command) {
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

static ezo_result_t ezo_rtd_scale_to_token(ezo_rtd_scale_t scale, const char **token_out) {
  if (token_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  switch (scale) {
  case EZO_RTD_SCALE_CELSIUS:
    *token_out = "c";
    return EZO_OK;
  case EZO_RTD_SCALE_KELVIN:
    *token_out = "k";
    return EZO_OK;
  case EZO_RTD_SCALE_FAHRENHEIT:
    *token_out = "f";
    return EZO_OK;
  case EZO_RTD_SCALE_UNKNOWN:
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }
}

static int ezo_rtd_is_space(char value) {
  return value == ' ' || value == '\t' || value == '\n' || value == '\r' ||
         value == '\f' || value == '\v';
}

static ezo_result_t ezo_rtd_send_i2c_command(ezo_i2c_device_t *device,
                                             const char *command,
                                             ezo_command_kind_t kind,
                                             ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_RTD,
                                                        EZO_PRODUCT_TRANSPORT_I2C,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_rtd_send_uart_command(ezo_uart_device_t *device,
                                              const char *command,
                                              ezo_command_kind_t kind,
                                              ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_RTD,
                                                        EZO_PRODUCT_TRANSPORT_UART,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_rtd_read_i2c_text(ezo_i2c_device_t *device,
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

static ezo_result_t ezo_rtd_read_uart_line_of_kind(ezo_uart_device_t *device,
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

static ezo_result_t ezo_rtd_read_uart_data_then_ok(ezo_uart_device_t *device,
                                                   char *buffer,
                                                   size_t buffer_len,
                                                   size_t *response_len) {
  char status_buffer[8];
  size_t status_len = 0;
  ezo_result_t result = ezo_rtd_read_uart_line_of_kind(device,
                                                       EZO_UART_RESPONSE_DATA,
                                                       buffer,
                                                       buffer_len,
                                                       response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_read_uart_line_of_kind(device,
                                        EZO_UART_RESPONSE_OK,
                                        status_buffer,
                                        sizeof(status_buffer),
                                        &status_len);
}

ezo_result_t ezo_rtd_parse_reading(const char *buffer,
                                   size_t buffer_len,
                                   ezo_rtd_scale_t scale,
                                   ezo_rtd_reading_t *reading_out) {
  ezo_scalar_reading_t reading;
  ezo_result_t result = EZO_OK;

  if (reading_out == NULL || scale == EZO_RTD_SCALE_UNKNOWN) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_schema_parse_scalar_reading(buffer,
                                           buffer_len,
                                           EZO_MEASUREMENT_FIELD_TEMPERATURE,
                                           &reading);
  if (result != EZO_OK) {
    return result;
  }

  reading_out->temperature = reading.value;
  reading_out->scale = scale;
  return EZO_OK;
}

ezo_result_t ezo_rtd_parse_scale(const char *buffer,
                                 size_t buffer_len,
                                 ezo_rtd_scale_status_t *status_out) {
  ezo_text_span_t fields[1];
  size_t field_count = 0;
  ezo_result_t result = EZO_OK;

  if (status_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_prefixed_fields(buffer, buffer_len, "?S", fields, 1, &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (field_count != 1) {
    return EZO_ERR_PARSE;
  }

  if (ezo_text_span_equals_cstr(fields[0], "c")) {
    status_out->scale = EZO_RTD_SCALE_CELSIUS;
    return EZO_OK;
  }

  if (ezo_text_span_equals_cstr(fields[0], "k")) {
    status_out->scale = EZO_RTD_SCALE_KELVIN;
    return EZO_OK;
  }

  if (ezo_text_span_equals_cstr(fields[0], "f")) {
    status_out->scale = EZO_RTD_SCALE_FAHRENHEIT;
    return EZO_OK;
  }

  return EZO_ERR_PARSE;
}

ezo_result_t ezo_rtd_parse_calibration_status(
    const char *buffer,
    size_t buffer_len,
    ezo_rtd_calibration_status_t *status_out) {
  ezo_text_span_t fields[1];
  size_t field_count = 0;
  uint32_t value = 0;
  ezo_result_t result = EZO_OK;

  if (status_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_prefixed_fields(buffer, buffer_len, "?Cal", fields, 1, &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (field_count != 1) {
    return EZO_ERR_PARSE;
  }

  result = ezo_parse_text_span_uint32(fields[0], &value);
  if (result != EZO_OK || value > 1U) {
    return EZO_ERR_PARSE;
  }

  status_out->calibrated = (uint8_t)value;
  return EZO_OK;
}

ezo_result_t ezo_rtd_parse_logger_status(const char *buffer,
                                         size_t buffer_len,
                                         ezo_rtd_logger_status_t *status_out) {
  ezo_text_span_t fields[1];
  size_t field_count = 0;
  ezo_result_t result = EZO_OK;

  if (status_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_prefixed_fields(buffer, buffer_len, "?D", fields, 1, &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (field_count != 1) {
    return EZO_ERR_PARSE;
  }

  return ezo_parse_text_span_uint32(fields[0], &status_out->interval_units);
}

ezo_result_t ezo_rtd_parse_memory_status(const char *buffer,
                                         size_t buffer_len,
                                         ezo_rtd_memory_status_t *status_out) {
  ezo_text_span_t fields[1];
  size_t field_count = 0;
  ezo_result_t result = EZO_OK;

  if (status_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_prefixed_fields(buffer, buffer_len, "?M", fields, 1, &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (field_count != 1) {
    return EZO_ERR_PARSE;
  }

  return ezo_parse_text_span_uint32(fields[0], &status_out->last_index);
}

ezo_result_t ezo_rtd_parse_memory_entry(const char *buffer,
                                        size_t buffer_len,
                                        ezo_rtd_scale_t scale,
                                        ezo_rtd_memory_entry_t *entry_out) {
  ezo_text_span_t fields[2];
  size_t field_count = 0;
  ezo_result_t result = EZO_OK;

  if (entry_out == NULL || scale == EZO_RTD_SCALE_UNKNOWN) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_csv_fields(buffer, buffer_len, fields, 2, &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (field_count != 2) {
    return EZO_ERR_PARSE;
  }

  result = ezo_parse_text_span_uint32(fields[0], &entry_out->index);
  if (result != EZO_OK) {
    return result;
  }

  result = ezo_parse_text_span_double(fields[1], &entry_out->temperature);
  if (result != EZO_OK) {
    return result;
  }

  entry_out->scale = scale;
  return EZO_OK;
}

ezo_result_t ezo_rtd_parse_memory_all(const char *buffer,
                                      size_t buffer_len,
                                      ezo_rtd_scale_t scale,
                                      ezo_rtd_memory_value_t *values_out,
                                      size_t values_capacity,
                                      size_t *value_count_out) {
  size_t start = 0;
  size_t end = buffer_len;
  size_t segment_start = 0;
  size_t i = 0;
  size_t value_count = 0;
  ezo_result_t result = EZO_OK;

  if (buffer == NULL || buffer_len == 0 || scale == EZO_RTD_SCALE_UNKNOWN ||
      values_out == NULL || values_capacity == 0 || value_count_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  while (start < end && ezo_rtd_is_space(buffer[start])) {
    start += 1;
  }

  while (end > start && ezo_rtd_is_space(buffer[end - 1U])) {
    end -= 1;
  }

  if (end <= start) {
    return EZO_ERR_PARSE;
  }

  segment_start = start;
  for (i = start; i <= end; ++i) {
    const int at_end = i == end;
    size_t field_start = segment_start;
    size_t field_end = i;

    if (!at_end && buffer[i] != ',') {
      continue;
    }

    if (value_count >= values_capacity) {
      return EZO_ERR_BUFFER_TOO_SMALL;
    }

    while (field_start < field_end && ezo_rtd_is_space(buffer[field_start])) {
      field_start += 1;
    }

    while (field_end > field_start && ezo_rtd_is_space(buffer[field_end - 1U])) {
      field_end -= 1;
    }

    if (field_end <= field_start) {
      return EZO_ERR_PARSE;
    }

    result = ezo_parse_double(buffer + field_start,
                              field_end - field_start,
                              &values_out[value_count].temperature);
    if (result != EZO_OK) {
      return result;
    }

    values_out[value_count].scale = scale;
    value_count += 1;
    segment_start = i + 1U;
  }

  *value_count_out = value_count;
  return EZO_OK;
}

ezo_result_t ezo_rtd_build_scale_command(char *buffer,
                                         size_t buffer_len,
                                         ezo_rtd_scale_t scale) {
  const char *token = NULL;
  char command[8];
  size_t used = 0;
  ezo_result_t result = ezo_rtd_scale_to_token(scale, &token);
  if (result != EZO_OK) {
    return result;
  }

  command[0] = '\0';
  result = ezo_rtd_copy_command(command, sizeof(command), "S,");
  if (result != EZO_OK) {
    return result;
  }

  used = strlen(command);
  if (used + 2U > sizeof(command)) {
    return EZO_ERR_BUFFER_TOO_SMALL;
  }

  command[used] = token[0];
  command[used + 1U] = '\0';
  return ezo_rtd_copy_command(buffer, buffer_len, command);
}

ezo_result_t ezo_rtd_build_calibration_command(char *buffer,
                                               size_t buffer_len,
                                               double reference_temperature,
                                               uint8_t decimals) {
  return ezo_common_format_fixed_command(buffer,
                                         buffer_len,
                                         "Cal,",
                                         reference_temperature,
                                         decimals);
}

ezo_result_t ezo_rtd_build_logger_command(char *buffer,
                                          size_t buffer_len,
                                          uint32_t interval_units) {
  return ezo_common_format_fixed_command(buffer, buffer_len, "D,", (double)interval_units, 0);
}

ezo_result_t ezo_rtd_send_read_i2c(ezo_i2c_device_t *device,
                                   ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_i2c_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_rtd_send_scale_query_i2c(ezo_i2c_device_t *device,
                                          ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_i2c_command(device, "S,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_scale_set_i2c(ezo_i2c_device_t *device,
                                        ezo_rtd_scale_t scale,
                                        ezo_timing_hint_t *timing_hint) {
  char command[8];
  ezo_result_t result = ezo_rtd_build_scale_command(command, sizeof(command), scale);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_calibration_query_i2c(ezo_i2c_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_i2c_command(device, "Cal,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_calibration_i2c(ezo_i2c_device_t *device,
                                          double reference_temperature,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result = ezo_rtd_build_calibration_command(command,
                                                          sizeof(command),
                                                          reference_temperature,
                                                          decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_send_i2c_command(device, command, EZO_COMMAND_CALIBRATION, timing_hint);
}

ezo_result_t ezo_rtd_send_clear_calibration_i2c(ezo_i2c_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_i2c_command(device, "Cal,clear", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_logger_query_i2c(ezo_i2c_device_t *device,
                                           ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_i2c_command(device, "D,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_logger_set_i2c(ezo_i2c_device_t *device,
                                         uint32_t interval_units,
                                         ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result =
      ezo_rtd_build_logger_command(command, sizeof(command), interval_units);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_memory_query_i2c(ezo_i2c_device_t *device,
                                           ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_i2c_command(device, "M,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_memory_next_i2c(ezo_i2c_device_t *device,
                                          ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_i2c_command(device, "M", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_memory_all_i2c(ezo_i2c_device_t *device,
                                         ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_i2c_command(device, "M,all", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_memory_clear_i2c(ezo_i2c_device_t *device,
                                           ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_i2c_command(device, "M,clear", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_read_response_i2c(ezo_i2c_device_t *device,
                                       ezo_rtd_scale_t scale,
                                       ezo_rtd_reading_t *reading_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_rtd_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_reading(buffer, response_len, scale, reading_out);
}

ezo_result_t ezo_rtd_read_scale_i2c(ezo_i2c_device_t *device,
                                    ezo_rtd_scale_status_t *status_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_rtd_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_scale(buffer, response_len, status_out);
}

ezo_result_t ezo_rtd_read_calibration_status_i2c(
    ezo_i2c_device_t *device,
    ezo_rtd_calibration_status_t *status_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_rtd_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_calibration_status(buffer, response_len, status_out);
}

ezo_result_t ezo_rtd_read_logger_i2c(ezo_i2c_device_t *device,
                                     ezo_rtd_logger_status_t *status_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_rtd_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_logger_status(buffer, response_len, status_out);
}

ezo_result_t ezo_rtd_read_memory_status_i2c(ezo_i2c_device_t *device,
                                            ezo_rtd_memory_status_t *status_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_rtd_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_memory_status(buffer, response_len, status_out);
}

ezo_result_t ezo_rtd_read_memory_entry_i2c(ezo_i2c_device_t *device,
                                           ezo_rtd_scale_t scale,
                                           ezo_rtd_memory_entry_t *entry_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_rtd_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_memory_entry(buffer, response_len, scale, entry_out);
}

ezo_result_t ezo_rtd_read_memory_all_i2c(ezo_i2c_device_t *device,
                                         ezo_rtd_scale_t scale,
                                         ezo_rtd_memory_value_t *values_out,
                                         size_t values_capacity,
                                         size_t *value_count_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_rtd_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_memory_all(buffer,
                                  response_len,
                                  scale,
                                  values_out,
                                  values_capacity,
                                  value_count_out);
}

ezo_result_t ezo_rtd_send_read_uart(ezo_uart_device_t *device,
                                    ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_uart_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_rtd_send_scale_query_uart(ezo_uart_device_t *device,
                                           ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_uart_command(device, "S,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_scale_set_uart(ezo_uart_device_t *device,
                                         ezo_rtd_scale_t scale,
                                         ezo_timing_hint_t *timing_hint) {
  char command[8];
  ezo_result_t result = ezo_rtd_build_scale_command(command, sizeof(command), scale);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_calibration_query_uart(ezo_uart_device_t *device,
                                                 ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_uart_command(device, "Cal,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_calibration_uart(ezo_uart_device_t *device,
                                           double reference_temperature,
                                           uint8_t decimals,
                                           ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result = ezo_rtd_build_calibration_command(command,
                                                          sizeof(command),
                                                          reference_temperature,
                                                          decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_send_uart_command(device, command, EZO_COMMAND_CALIBRATION, timing_hint);
}

ezo_result_t ezo_rtd_send_clear_calibration_uart(ezo_uart_device_t *device,
                                                 ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_uart_command(device, "Cal,clear", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_logger_query_uart(ezo_uart_device_t *device,
                                            ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_uart_command(device, "D,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_logger_set_uart(ezo_uart_device_t *device,
                                          uint32_t interval_units,
                                          ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result =
      ezo_rtd_build_logger_command(command, sizeof(command), interval_units);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_memory_query_uart(ezo_uart_device_t *device,
                                            ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_uart_command(device, "M,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_memory_next_uart(ezo_uart_device_t *device,
                                           ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_uart_command(device, "M", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_memory_all_uart(ezo_uart_device_t *device,
                                          ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_uart_command(device, "M,all", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_send_memory_clear_uart(ezo_uart_device_t *device,
                                            ezo_timing_hint_t *timing_hint) {
  return ezo_rtd_send_uart_command(device, "M,clear", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_rtd_read_response_uart(ezo_uart_device_t *device,
                                        ezo_rtd_scale_t scale,
                                        ezo_rtd_reading_t *reading_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_rtd_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_reading(buffer, response_len, scale, reading_out);
}

ezo_result_t ezo_rtd_read_scale_uart(ezo_uart_device_t *device,
                                     ezo_rtd_scale_status_t *status_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_rtd_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_scale(buffer, response_len, status_out);
}

ezo_result_t ezo_rtd_read_calibration_status_uart(
    ezo_uart_device_t *device,
    ezo_rtd_calibration_status_t *status_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_rtd_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_calibration_status(buffer, response_len, status_out);
}

ezo_result_t ezo_rtd_read_logger_uart(ezo_uart_device_t *device,
                                      ezo_rtd_logger_status_t *status_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_rtd_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_logger_status(buffer, response_len, status_out);
}

ezo_result_t ezo_rtd_read_memory_status_uart(ezo_uart_device_t *device,
                                             ezo_rtd_memory_status_t *status_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_rtd_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_memory_status(buffer, response_len, status_out);
}

ezo_result_t ezo_rtd_read_memory_entry_uart(ezo_uart_device_t *device,
                                            ezo_rtd_scale_t scale,
                                            ezo_rtd_memory_entry_t *entry_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_rtd_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_memory_entry(buffer, response_len, scale, entry_out);
}

ezo_result_t ezo_rtd_read_memory_all_uart(ezo_uart_device_t *device,
                                          ezo_rtd_scale_t scale,
                                          ezo_rtd_memory_value_t *values_out,
                                          size_t values_capacity,
                                          size_t *value_count_out) {
  char buffer[EZO_RTD_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_rtd_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_rtd_parse_memory_all(buffer,
                                  response_len,
                                  scale,
                                  values_out,
                                  values_capacity,
                                  value_count_out);
}
