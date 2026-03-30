#include "ezo_ph.h"

#include "ezo_common.h"
#include "ezo_parse.h"
#include "ezo_product.h"
#include "ezo_schema.h"

#include <string.h>

enum {
  EZO_PH_RESPONSE_BUFFER_LEN = 64
};

static ezo_result_t ezo_ph_copy_command(char *buffer, size_t buffer_len, const char *command) {
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

static ezo_result_t ezo_ph_send_i2c_command(ezo_i2c_device_t *device,
                                            const char *command,
                                            ezo_command_kind_t kind,
                                            ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_PH,
                                                        EZO_PRODUCT_TRANSPORT_I2C,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_ph_send_uart_command(ezo_uart_device_t *device,
                                             const char *command,
                                             ezo_command_kind_t kind,
                                             ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_PH,
                                                        EZO_PRODUCT_TRANSPORT_UART,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_ph_send_i2c_float_command(ezo_i2c_device_t *device,
                                                  const char *prefix,
                                                  double value,
                                                  uint8_t decimals,
                                                  ezo_command_kind_t kind,
                                                  ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_PH,
                                                        EZO_PRODUCT_TRANSPORT_I2C,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command_with_float(device, prefix, value, decimals, kind, NULL);
}

static ezo_result_t ezo_ph_send_uart_float_command(ezo_uart_device_t *device,
                                                   const char *prefix,
                                                   double value,
                                                   uint8_t decimals,
                                                   ezo_command_kind_t kind,
                                                   ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_PH,
                                                        EZO_PRODUCT_TRANSPORT_UART,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command_with_float(device, prefix, value, decimals, kind, NULL);
}

static ezo_result_t ezo_ph_read_i2c_text(ezo_i2c_device_t *device,
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

static ezo_result_t ezo_ph_read_uart_line_of_kind(ezo_uart_device_t *device,
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

static ezo_result_t ezo_ph_read_uart_data_line(ezo_uart_device_t *device,
                                               char *buffer,
                                               size_t buffer_len,
                                               size_t *response_len) {
  return ezo_ph_read_uart_line_of_kind(device,
                                       EZO_UART_RESPONSE_DATA,
                                       buffer,
                                       buffer_len,
                                       response_len);
}

static ezo_result_t ezo_ph_expect_uart_ok(ezo_uart_device_t *device) {
  return ezo_uart_read_ok(device);
}

static ezo_result_t ezo_ph_read_uart_data_then_ok(ezo_uart_device_t *device,
                                                  char *buffer,
                                                  size_t buffer_len,
                                                  size_t *response_len) {
  ezo_result_t result = ezo_ph_read_uart_data_line(device, buffer, buffer_len, response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_expect_uart_ok(device);
}

static ezo_result_t ezo_ph_read_uart_ok_then_data(ezo_uart_device_t *device,
                                                  char *buffer,
                                                  size_t buffer_len,
                                                  size_t *response_len) {
  ezo_result_t result = ezo_ph_expect_uart_ok(device);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_read_uart_data_line(device, buffer, buffer_len, response_len);
}

static ezo_result_t ezo_ph_parse_query_double(const char *buffer,
                                              size_t buffer_len,
                                              const char *prefix,
                                              double *value_out) {
  ezo_text_span_t fields[1];
  size_t field_count = 0;
  ezo_result_t result =
      ezo_parse_prefixed_fields(buffer, buffer_len, prefix, fields, 1, &field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (field_count != 1) {
    return EZO_ERR_PARSE;
  }

  return ezo_parse_text_span_double(fields[0], value_out);
}

ezo_result_t ezo_ph_parse_reading(const char *buffer,
                                  size_t buffer_len,
                                  ezo_ph_reading_t *reading_out) {
  ezo_scalar_reading_t reading;
  ezo_result_t result = EZO_OK;

  if (reading_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_schema_parse_scalar_reading(buffer, buffer_len, EZO_MEASUREMENT_FIELD_PH, &reading);
  if (result != EZO_OK) {
    return result;
  }

  reading_out->ph = reading.value;
  return EZO_OK;
}

ezo_result_t ezo_ph_parse_temperature(const char *buffer,
                                      size_t buffer_len,
                                      ezo_ph_temperature_compensation_t *temperature_out) {
  if (temperature_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return ezo_ph_parse_query_double(buffer,
                                   buffer_len,
                                   "?T",
                                   &temperature_out->temperature_c);
}

ezo_result_t ezo_ph_parse_calibration_status(const char *buffer,
                                             size_t buffer_len,
                                             ezo_ph_calibration_status_t *status_out) {
  ezo_text_span_t fields[1];
  size_t field_count = 0;
  uint32_t value = 0;
  ezo_result_t result = EZO_OK;

  if (status_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_prefixed_fields(buffer, buffer_len, "?Cal", fields, 1, &field_count);
  if (result != EZO_OK) {
    result = ezo_parse_prefixed_fields(buffer, buffer_len, "?CAL", fields, 1, &field_count);
  }
  if (result != EZO_OK) {
    return result;
  }

  if (field_count != 1) {
    return EZO_ERR_PARSE;
  }

  result = ezo_parse_text_span_uint32(fields[0], &value);
  if (result != EZO_OK || value > 3U) {
    return EZO_ERR_PARSE;
  }

  status_out->level = (ezo_ph_calibration_level_t)value;
  return EZO_OK;
}

ezo_result_t ezo_ph_parse_slope(const char *buffer,
                                size_t buffer_len,
                                ezo_ph_slope_t *slope_out) {
  ezo_text_span_t fields[3];
  size_t field_count = 0;
  ezo_result_t result = EZO_OK;

  if (slope_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_prefixed_fields(buffer, buffer_len, "?Slope", fields, 3, &field_count);
  if (result != EZO_OK) {
    result = ezo_parse_prefixed_fields(buffer, buffer_len, "?SLOPE", fields, 3, &field_count);
  }
  if (result != EZO_OK) {
    return result;
  }

  if (field_count != 3) {
    return EZO_ERR_PARSE;
  }

  result = ezo_parse_text_span_double(fields[0], &slope_out->acid_percent);
  if (result != EZO_OK) {
    return result;
  }

  result = ezo_parse_text_span_double(fields[1], &slope_out->base_percent);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_parse_text_span_double(fields[2], &slope_out->neutral_mv);
}

ezo_result_t ezo_ph_parse_extended_range(const char *buffer,
                                         size_t buffer_len,
                                         ezo_ph_extended_range_status_t *status_out) {
  ezo_text_span_t fields[1];
  size_t field_count = 0;
  uint32_t value = 0;
  ezo_result_t result = EZO_OK;

  if (status_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_prefixed_fields(buffer, buffer_len, "?pHext", fields, 1, &field_count);
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

  status_out->enabled = (ezo_ph_extended_range_t)value;
  return EZO_OK;
}

ezo_result_t ezo_ph_build_temperature_command(char *buffer,
                                              size_t buffer_len,
                                              double temperature_c,
                                              uint8_t decimals) {
  return ezo_common_format_fixed_command(buffer, buffer_len, "T,", temperature_c, decimals);
}

ezo_result_t ezo_ph_build_calibration_command(char *buffer,
                                              size_t buffer_len,
                                              ezo_ph_calibration_point_t point,
                                              double reference_ph,
                                              uint8_t decimals) {
  const char *prefix = NULL;

  switch (point) {
  case EZO_PH_CALIBRATION_POINT_MID:
    prefix = "Cal,mid,";
    break;
  case EZO_PH_CALIBRATION_POINT_LOW:
    prefix = "Cal,low,";
    break;
  case EZO_PH_CALIBRATION_POINT_HIGH:
    prefix = "Cal,high,";
    break;
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return ezo_common_format_fixed_command(buffer, buffer_len, prefix, reference_ph, decimals);
}

ezo_result_t ezo_ph_build_extended_range_command(char *buffer,
                                                 size_t buffer_len,
                                                 ezo_ph_extended_range_t enabled) {
  switch (enabled) {
  case EZO_PH_EXTENDED_RANGE_DISABLED:
    return ezo_ph_copy_command(buffer, buffer_len, "pHext,0");
  case EZO_PH_EXTENDED_RANGE_ENABLED:
    return ezo_ph_copy_command(buffer, buffer_len, "pHext,1");
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }
}

ezo_result_t ezo_ph_send_read_i2c(ezo_i2c_device_t *device,
                                  ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_i2c_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_ph_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device,
                                                 double temperature_c,
                                                 uint8_t decimals,
                                                 ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_i2c_float_command(device,
                                       "rt,",
                                       temperature_c,
                                       decimals,
                                       EZO_COMMAND_READ_WITH_TEMP_COMP,
                                       timing_hint);
}

ezo_result_t ezo_ph_send_temperature_query_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_i2c_command(device, "T,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_temperature_set_i2c(ezo_i2c_device_t *device,
                                             double temperature_c,
                                             uint8_t decimals,
                                             ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ph_build_temperature_command(command, sizeof(command), temperature_c, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_calibration_query_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_i2c_command(device, "Cal,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_calibration_i2c(ezo_i2c_device_t *device,
                                         ezo_ph_calibration_point_t point,
                                         double reference_ph,
                                         uint8_t decimals,
                                         ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ph_build_calibration_command(command, sizeof(command), point, reference_ph, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_send_i2c_command(device, command, EZO_COMMAND_CALIBRATION, timing_hint);
}

ezo_result_t ezo_ph_send_clear_calibration_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_i2c_command(device, "Cal,clear", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_slope_query_i2c(ezo_i2c_device_t *device,
                                         ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_i2c_command(device, "Slope,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_extended_range_query_i2c(ezo_i2c_device_t *device,
                                                  ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_i2c_command(device, "pHext,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_extended_range_set_i2c(ezo_i2c_device_t *device,
                                                ezo_ph_extended_range_t enabled,
                                                ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result =
      ezo_ph_build_extended_range_command(command, sizeof(command), enabled);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_read_response_i2c(ezo_i2c_device_t *device,
                                      ezo_ph_reading_t *reading_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ph_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_reading(buffer, response_len, reading_out);
}

ezo_result_t ezo_ph_read_temperature_i2c(ezo_i2c_device_t *device,
                                         ezo_ph_temperature_compensation_t *temperature_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ph_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_temperature(buffer, response_len, temperature_out);
}

ezo_result_t ezo_ph_read_calibration_status_i2c(ezo_i2c_device_t *device,
                                                ezo_ph_calibration_status_t *status_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ph_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_calibration_status(buffer, response_len, status_out);
}

ezo_result_t ezo_ph_read_slope_i2c(ezo_i2c_device_t *device,
                                   ezo_ph_slope_t *slope_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ph_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_slope(buffer, response_len, slope_out);
}

ezo_result_t ezo_ph_read_extended_range_i2c(ezo_i2c_device_t *device,
                                            ezo_ph_extended_range_status_t *status_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ph_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_extended_range(buffer, response_len, status_out);
}

ezo_result_t ezo_ph_send_read_uart(ezo_uart_device_t *device,
                                   ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_uart_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_ph_send_read_with_temp_comp_uart(ezo_uart_device_t *device,
                                                  double temperature_c,
                                                  uint8_t decimals,
                                                  ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_uart_float_command(device,
                                        "rt,",
                                        temperature_c,
                                        decimals,
                                        EZO_COMMAND_READ_WITH_TEMP_COMP,
                                        timing_hint);
}

ezo_result_t ezo_ph_send_temperature_query_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_uart_command(device, "T,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_temperature_set_uart(ezo_uart_device_t *device,
                                              double temperature_c,
                                              uint8_t decimals,
                                              ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ph_build_temperature_command(command, sizeof(command), temperature_c, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_calibration_query_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_uart_command(device, "Cal,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_calibration_uart(ezo_uart_device_t *device,
                                          ezo_ph_calibration_point_t point,
                                          double reference_ph,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ph_build_calibration_command(command, sizeof(command), point, reference_ph, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_send_uart_command(device, command, EZO_COMMAND_CALIBRATION, timing_hint);
}

ezo_result_t ezo_ph_send_clear_calibration_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_uart_command(device, "Cal,clear", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_slope_query_uart(ezo_uart_device_t *device,
                                          ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_uart_command(device, "Slope,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_extended_range_query_uart(ezo_uart_device_t *device,
                                                   ezo_timing_hint_t *timing_hint) {
  return ezo_ph_send_uart_command(device, "pHext,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_send_extended_range_set_uart(ezo_uart_device_t *device,
                                                 ezo_ph_extended_range_t enabled,
                                                 ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result =
      ezo_ph_build_extended_range_command(command, sizeof(command), enabled);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ph_read_response_uart(ezo_uart_device_t *device,
                                       ezo_ph_reading_t *reading_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ph_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_reading(buffer, response_len, reading_out);
}

ezo_result_t ezo_ph_read_response_with_temp_comp_uart(
    ezo_uart_device_t *device,
    ezo_ph_reading_t *reading_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ph_read_uart_ok_then_data(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_reading(buffer, response_len, reading_out);
}

ezo_result_t ezo_ph_read_temperature_uart(
    ezo_uart_device_t *device,
    ezo_ph_temperature_compensation_t *temperature_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ph_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_temperature(buffer, response_len, temperature_out);
}

ezo_result_t ezo_ph_read_calibration_status_uart(
    ezo_uart_device_t *device,
    ezo_ph_calibration_status_t *status_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ph_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_calibration_status(buffer, response_len, status_out);
}

ezo_result_t ezo_ph_read_slope_uart(ezo_uart_device_t *device,
                                    ezo_ph_slope_t *slope_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ph_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_slope(buffer, response_len, slope_out);
}

ezo_result_t ezo_ph_read_extended_range_uart(ezo_uart_device_t *device,
                                             ezo_ph_extended_range_status_t *status_out) {
  char buffer[EZO_PH_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ph_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ph_parse_extended_range(buffer, response_len, status_out);
}
