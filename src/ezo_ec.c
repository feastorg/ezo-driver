#include "ezo_ec.h"

#include "ezo_common.h"
#include "ezo_parse.h"
#include "ezo_product.h"
#include "ezo_schema.h"

#include <string.h>

enum {
  EZO_EC_RESPONSE_BUFFER_LEN = 64
};

static ezo_result_t ezo_ec_copy_command(char *buffer, size_t buffer_len, const char *command) {
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

static ezo_result_t ezo_ec_send_i2c_command(ezo_i2c_device_t *device,
                                            const char *command,
                                            ezo_command_kind_t kind,
                                            ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_EC,
                                                        EZO_PRODUCT_TRANSPORT_I2C,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_ec_send_uart_command(ezo_uart_device_t *device,
                                             const char *command,
                                             ezo_command_kind_t kind,
                                             ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_EC,
                                                        EZO_PRODUCT_TRANSPORT_UART,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command(device, command, kind, NULL);
}

static ezo_result_t ezo_ec_send_i2c_float_command(ezo_i2c_device_t *device,
                                                  const char *prefix,
                                                  double value,
                                                  uint8_t decimals,
                                                  ezo_command_kind_t kind,
                                                  ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_EC,
                                                        EZO_PRODUCT_TRANSPORT_I2C,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command_with_float(device, prefix, value, decimals, kind, NULL);
}

static ezo_result_t ezo_ec_send_uart_float_command(ezo_uart_device_t *device,
                                                   const char *prefix,
                                                   double value,
                                                   uint8_t decimals,
                                                   ezo_command_kind_t kind,
                                                   ezo_timing_hint_t *timing_hint) {
  ezo_timing_hint_t local_hint;
  ezo_result_t result = ezo_product_resolve_timing_hint(EZO_PRODUCT_EC,
                                                        EZO_PRODUCT_TRANSPORT_UART,
                                                        kind,
                                                        timing_hint != NULL ? timing_hint
                                                                            : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command_with_float(device, prefix, value, decimals, kind, NULL);
}

static ezo_result_t ezo_ec_read_i2c_text(ezo_i2c_device_t *device,
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

static ezo_result_t ezo_ec_read_uart_line_of_kind(ezo_uart_device_t *device,
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

static ezo_result_t ezo_ec_read_uart_data_line(ezo_uart_device_t *device,
                                               char *buffer,
                                               size_t buffer_len,
                                               size_t *response_len) {
  return ezo_ec_read_uart_line_of_kind(device,
                                       EZO_UART_RESPONSE_DATA,
                                       buffer,
                                       buffer_len,
                                       response_len);
}

static ezo_result_t ezo_ec_read_uart_data_then_ok(ezo_uart_device_t *device,
                                                  char *buffer,
                                                  size_t buffer_len,
                                                  size_t *response_len) {
  char status_buffer[8];
  size_t status_len = 0;
  ezo_result_t result = ezo_ec_read_uart_data_line(device, buffer, buffer_len, response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_read_uart_line_of_kind(device,
                                       EZO_UART_RESPONSE_OK,
                                       status_buffer,
                                       sizeof(status_buffer),
                                       &status_len);
}

static ezo_result_t ezo_ec_parse_query_double(const char *buffer,
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

static ezo_result_t ezo_ec_parse_output_field(ezo_text_span_t field,
                                              ezo_ec_output_mask_t *mask_out) {
  if (mask_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (ezo_text_span_equals_cstr(field, "EC")) {
    *mask_out = EZO_EC_OUTPUT_CONDUCTIVITY;
    return EZO_OK;
  }

  if (ezo_text_span_equals_cstr(field, "TDS")) {
    *mask_out = EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS;
    return EZO_OK;
  }

  if (ezo_text_span_equals_cstr(field, "S")) {
    *mask_out = EZO_EC_OUTPUT_SALINITY;
    return EZO_OK;
  }

  if (ezo_text_span_equals_cstr(field, "SG")) {
    *mask_out = EZO_EC_OUTPUT_SPECIFIC_GRAVITY;
    return EZO_OK;
  }

  return EZO_ERR_PARSE;
}

ezo_result_t ezo_ec_parse_reading(const char *buffer,
                                  size_t buffer_len,
                                  ezo_ec_output_mask_t enabled_mask,
                                  ezo_ec_reading_t *reading_out) {
  ezo_output_schema_t schema;
  ezo_multi_output_reading_t reading;
  ezo_result_t result = EZO_OK;

  if (reading_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_schema_get_output_schema(EZO_PRODUCT_EC, &schema);
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
  reading_out->conductivity_us_cm = reading.values[0].value;
  reading_out->total_dissolved_solids_ppm = reading.values[1].value;
  reading_out->salinity_ppt = reading.values[2].value;
  reading_out->specific_gravity = reading.values[3].value;
  return EZO_OK;
}

ezo_result_t ezo_ec_parse_output_config(const char *buffer,
                                        size_t buffer_len,
                                        ezo_ec_output_config_t *config_out) {
  ezo_text_span_t prefix;
  ezo_text_span_t fields[EZO_SCHEMA_MAX_FIELDS];
  size_t field_count = 0;
  size_t i = 0;
  ezo_result_t result = EZO_OK;
  ezo_ec_output_mask_t enabled_mask = 0;

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
    ezo_ec_output_mask_t output = 0;
    result = ezo_ec_parse_output_field(fields[i], &output);
    if (result != EZO_OK) {
      return result;
    }
    enabled_mask |= output;
  }

  config_out->enabled_mask = enabled_mask;
  return EZO_OK;
}

ezo_result_t ezo_ec_parse_temperature(const char *buffer,
                                      size_t buffer_len,
                                      ezo_ec_temperature_compensation_t *temperature_out) {
  if (temperature_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return ezo_ec_parse_query_double(buffer,
                                   buffer_len,
                                   "?T",
                                   &temperature_out->temperature_c);
}

ezo_result_t ezo_ec_parse_probe_k(const char *buffer,
                                  size_t buffer_len,
                                  ezo_ec_probe_k_t *probe_k_out) {
  if (probe_k_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return ezo_ec_parse_query_double(buffer, buffer_len, "?K", &probe_k_out->k_value);
}

ezo_result_t ezo_ec_parse_tds_factor(const char *buffer,
                                     size_t buffer_len,
                                     ezo_ec_tds_factor_t *tds_factor_out) {
  if (tds_factor_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return ezo_ec_parse_query_double(buffer, buffer_len, "?TDS", &tds_factor_out->factor);
}

ezo_result_t ezo_ec_parse_calibration_status(const char *buffer,
                                             size_t buffer_len,
                                             ezo_ec_calibration_status_t *status_out) {
  ezo_text_span_t fields[1];
  size_t field_count = 0;
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

  return ezo_parse_text_span_uint32(fields[0], &status_out->level);
}

ezo_result_t ezo_ec_build_output_command(char *buffer,
                                         size_t buffer_len,
                                         ezo_ec_output_mask_t output,
                                         uint8_t enabled) {
  switch (output) {
  case EZO_EC_OUTPUT_CONDUCTIVITY:
    return ezo_ec_copy_command(buffer, buffer_len, enabled != 0 ? "O,EC,1" : "O,EC,0");
  case EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS:
    return ezo_ec_copy_command(buffer, buffer_len, enabled != 0 ? "O,TDS,1" : "O,TDS,0");
  case EZO_EC_OUTPUT_SALINITY:
    return ezo_ec_copy_command(buffer, buffer_len, enabled != 0 ? "O,S,1" : "O,S,0");
  case EZO_EC_OUTPUT_SPECIFIC_GRAVITY:
    return ezo_ec_copy_command(buffer, buffer_len, enabled != 0 ? "O,SG,1" : "O,SG,0");
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }
}

ezo_result_t ezo_ec_build_temperature_command(char *buffer,
                                              size_t buffer_len,
                                              double temperature_c,
                                              uint8_t decimals) {
  return ezo_common_format_fixed_command(buffer, buffer_len, "T,", temperature_c, decimals);
}

ezo_result_t ezo_ec_build_probe_k_command(char *buffer,
                                          size_t buffer_len,
                                          double k_value,
                                          uint8_t decimals) {
  return ezo_common_format_fixed_command(buffer, buffer_len, "K,", k_value, decimals);
}

ezo_result_t ezo_ec_build_tds_factor_command(char *buffer,
                                             size_t buffer_len,
                                             double factor,
                                             uint8_t decimals) {
  return ezo_common_format_fixed_command(buffer, buffer_len, "TDS,", factor, decimals);
}

ezo_result_t ezo_ec_build_calibration_command(char *buffer,
                                              size_t buffer_len,
                                              ezo_ec_calibration_point_t point,
                                              double reference_value,
                                              uint8_t decimals) {
  switch (point) {
  case EZO_EC_CALIBRATION_DRY:
    return ezo_ec_copy_command(buffer, buffer_len, "Cal,dry");
  case EZO_EC_CALIBRATION_SINGLE_POINT:
    return ezo_common_format_fixed_command(buffer,
                                           buffer_len,
                                           "Cal,",
                                           reference_value,
                                           decimals);
  case EZO_EC_CALIBRATION_LOW_POINT:
    return ezo_common_format_fixed_command(buffer,
                                           buffer_len,
                                           "Cal,low,",
                                           reference_value,
                                           decimals);
  case EZO_EC_CALIBRATION_HIGH_POINT:
    return ezo_common_format_fixed_command(buffer,
                                           buffer_len,
                                           "Cal,high,",
                                           reference_value,
                                           decimals);
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }
}

ezo_result_t ezo_ec_send_read_i2c(ezo_i2c_device_t *device,
                                  ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_i2c_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_ec_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device,
                                                 double temperature_c,
                                                 uint8_t decimals,
                                                 ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_i2c_float_command(device,
                                       "rt,",
                                       temperature_c,
                                       decimals,
                                       EZO_COMMAND_READ_WITH_TEMP_COMP,
                                       timing_hint);
}

ezo_result_t ezo_ec_send_output_query_i2c(ezo_i2c_device_t *device,
                                          ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_i2c_command(device, "O,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_output_set_i2c(ezo_i2c_device_t *device,
                                        ezo_ec_output_mask_t output,
                                        uint8_t enabled,
                                        ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result = ezo_ec_build_output_command(command, sizeof(command), output, enabled);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_temperature_query_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_i2c_command(device, "T,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_temperature_set_i2c(ezo_i2c_device_t *device,
                                             double temperature_c,
                                             uint8_t decimals,
                                             ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ec_build_temperature_command(command, sizeof(command), temperature_c, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_probe_k_query_i2c(ezo_i2c_device_t *device,
                                           ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_i2c_command(device, "K,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_probe_k_set_i2c(ezo_i2c_device_t *device,
                                         double k_value,
                                         uint8_t decimals,
                                         ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ec_build_probe_k_command(command, sizeof(command), k_value, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_tds_factor_query_i2c(ezo_i2c_device_t *device,
                                              ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_i2c_command(device, "TDS,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_tds_factor_set_i2c(ezo_i2c_device_t *device,
                                            double factor,
                                            uint8_t decimals,
                                            ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ec_build_tds_factor_command(command, sizeof(command), factor, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_i2c_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_calibration_query_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_i2c_command(device, "Cal,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_calibration_i2c(ezo_i2c_device_t *device,
                                         ezo_ec_calibration_point_t point,
                                         double reference_value,
                                         uint8_t decimals,
                                         ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result = ezo_ec_build_calibration_command(command,
                                                         sizeof(command),
                                                         point,
                                                         reference_value,
                                                         decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_i2c_command(device, command, EZO_COMMAND_CALIBRATION, timing_hint);
}

ezo_result_t ezo_ec_send_clear_calibration_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_i2c_command(device, "Cal,clear", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_read_response_i2c(ezo_i2c_device_t *device,
                                      ezo_ec_output_mask_t enabled_mask,
                                      ezo_ec_reading_t *reading_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ec_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_reading(buffer, response_len, enabled_mask, reading_out);
}

ezo_result_t ezo_ec_read_output_config_i2c(ezo_i2c_device_t *device,
                                           ezo_ec_output_config_t *config_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ec_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_output_config(buffer, response_len, config_out);
}

ezo_result_t ezo_ec_read_temperature_i2c(ezo_i2c_device_t *device,
                                         ezo_ec_temperature_compensation_t *temperature_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ec_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_temperature(buffer, response_len, temperature_out);
}

ezo_result_t ezo_ec_read_probe_k_i2c(ezo_i2c_device_t *device,
                                     ezo_ec_probe_k_t *probe_k_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ec_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_probe_k(buffer, response_len, probe_k_out);
}

ezo_result_t ezo_ec_read_tds_factor_i2c(ezo_i2c_device_t *device,
                                        ezo_ec_tds_factor_t *tds_factor_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ec_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_tds_factor(buffer, response_len, tds_factor_out);
}

ezo_result_t ezo_ec_read_calibration_status_i2c(ezo_i2c_device_t *device,
                                                ezo_ec_calibration_status_t *status_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result = ezo_ec_read_i2c_text(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_calibration_status(buffer, response_len, status_out);
}

ezo_result_t ezo_ec_send_read_uart(ezo_uart_device_t *device,
                                   ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_uart_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_ec_send_read_with_temp_comp_uart(ezo_uart_device_t *device,
                                                  double temperature_c,
                                                  uint8_t decimals,
                                                  ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_uart_float_command(device,
                                        "rt,",
                                        temperature_c,
                                        decimals,
                                        EZO_COMMAND_READ_WITH_TEMP_COMP,
                                        timing_hint);
}

ezo_result_t ezo_ec_send_output_query_uart(ezo_uart_device_t *device,
                                           ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_uart_command(device, "O,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_output_set_uart(ezo_uart_device_t *device,
                                         ezo_ec_output_mask_t output,
                                         uint8_t enabled,
                                         ezo_timing_hint_t *timing_hint) {
  char command[16];
  ezo_result_t result = ezo_ec_build_output_command(command, sizeof(command), output, enabled);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_temperature_query_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_uart_command(device, "T,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_temperature_set_uart(ezo_uart_device_t *device,
                                              double temperature_c,
                                              uint8_t decimals,
                                              ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ec_build_temperature_command(command, sizeof(command), temperature_c, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_probe_k_query_uart(ezo_uart_device_t *device,
                                            ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_uart_command(device, "K,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_probe_k_set_uart(ezo_uart_device_t *device,
                                          double k_value,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ec_build_probe_k_command(command, sizeof(command), k_value, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_tds_factor_query_uart(ezo_uart_device_t *device,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_uart_command(device, "TDS,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_tds_factor_set_uart(ezo_uart_device_t *device,
                                             double factor,
                                             uint8_t decimals,
                                             ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result =
      ezo_ec_build_tds_factor_command(command, sizeof(command), factor, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_uart_command(device, command, EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_calibration_query_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_uart_command(device, "Cal,?", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_send_calibration_uart(ezo_uart_device_t *device,
                                          ezo_ec_calibration_point_t point,
                                          double reference_value,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint) {
  char command[32];
  ezo_result_t result = ezo_ec_build_calibration_command(command,
                                                         sizeof(command),
                                                         point,
                                                         reference_value,
                                                         decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_send_uart_command(device, command, EZO_COMMAND_CALIBRATION, timing_hint);
}

ezo_result_t ezo_ec_send_clear_calibration_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint) {
  return ezo_ec_send_uart_command(device, "Cal,clear", EZO_COMMAND_GENERIC, timing_hint);
}

ezo_result_t ezo_ec_read_response_uart(ezo_uart_device_t *device,
                                       ezo_ec_output_mask_t enabled_mask,
                                       ezo_ec_reading_t *reading_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ec_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_reading(buffer, response_len, enabled_mask, reading_out);
}

ezo_result_t ezo_ec_read_output_config_uart(ezo_uart_device_t *device,
                                            ezo_ec_output_config_t *config_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ec_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_output_config(buffer, response_len, config_out);
}

ezo_result_t ezo_ec_read_temperature_uart(
    ezo_uart_device_t *device,
    ezo_ec_temperature_compensation_t *temperature_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ec_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_temperature(buffer, response_len, temperature_out);
}

ezo_result_t ezo_ec_read_probe_k_uart(ezo_uart_device_t *device,
                                      ezo_ec_probe_k_t *probe_k_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ec_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_probe_k(buffer, response_len, probe_k_out);
}

ezo_result_t ezo_ec_read_tds_factor_uart(ezo_uart_device_t *device,
                                         ezo_ec_tds_factor_t *tds_factor_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ec_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_tds_factor(buffer, response_len, tds_factor_out);
}

ezo_result_t ezo_ec_read_calibration_status_uart(ezo_uart_device_t *device,
                                                 ezo_ec_calibration_status_t *status_out) {
  char buffer[EZO_EC_RESPONSE_BUFFER_LEN];
  size_t response_len = 0;
  ezo_result_t result =
      ezo_ec_read_uart_data_then_ok(device, buffer, sizeof(buffer), &response_len);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_ec_parse_calibration_status(buffer, response_len, status_out);
}
