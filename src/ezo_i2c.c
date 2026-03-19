#include "ezo_i2c/ezo_i2c.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>

enum {
  EZO_I2C_MAX_RAW_RESPONSE_LEN = EZO_I2C_MAX_TEXT_RESPONSE_LEN + 1,
  EZO_I2C_MAX_FORMAT_DECIMALS = 9
};

static ezo_device_status_t ezo_map_status_byte(uint8_t status_byte) {
  switch (status_byte) {
  case 1:
    return EZO_STATUS_SUCCESS;
  case 2:
    return EZO_STATUS_FAIL;
  case 254:
    return EZO_STATUS_NOT_READY;
  case 255:
    return EZO_STATUS_NO_DATA;
  default:
    return EZO_STATUS_UNKNOWN;
  }
}

static ezo_result_t ezo_append_char(char *buffer, size_t buffer_len, size_t *used, char value) {
  if (buffer == NULL || used == NULL || buffer_len == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (*used + 1 >= buffer_len) {
    return EZO_ERR_BUFFER_TOO_SMALL;
  }

  buffer[*used] = value;
  *used += 1;
  buffer[*used] = '\0';
  return EZO_OK;
}

static ezo_result_t ezo_append_cstr(char *buffer,
                                    size_t buffer_len,
                                    size_t *used,
                                    const char *value) {
  size_t i = 0;

  if (value == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  for (i = 0; value[i] != '\0'; ++i) {
    ezo_result_t result = ezo_append_char(buffer, buffer_len, used, value[i]);
    if (result != EZO_OK) {
      return result;
    }
  }

  return EZO_OK;
}

static ezo_result_t ezo_append_uint64(char *buffer,
                                      size_t buffer_len,
                                      size_t *used,
                                      uint64_t value,
                                      uint8_t min_digits) {
  char digits[32];
  uint8_t count = 0;
  uint8_t i = 0;

  if (min_digits > sizeof(digits)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  do {
    digits[count] = (char)('0' + (value % 10U));
    value /= 10U;
    count += 1;
  } while (value > 0 && count < sizeof(digits));

  while (count < min_digits) {
    digits[count] = '0';
    count += 1;
  }

  for (i = 0; i < count; ++i) {
    ezo_result_t result = ezo_append_char(buffer, buffer_len, used, digits[count - 1U - i]);
    if (result != EZO_OK) {
      return result;
    }
  }

  return EZO_OK;
}

static ezo_result_t ezo_format_fixed_command(char *buffer,
                                             size_t buffer_len,
                                             const char *prefix,
                                             double value,
                                             uint8_t decimals) {
  uint64_t scale = 1;
  uint8_t i = 0;
  size_t used = 0;
  int is_negative = 0;
  double absolute_value = value;
  double scaled_value = 0.0;
  uint64_t scaled_integer = 0;
  uint64_t integer_part = 0;
  uint64_t fractional_part = 0;
  ezo_result_t result = EZO_OK;

  if (buffer == NULL || buffer_len == 0 || prefix == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (decimals > EZO_I2C_MAX_FORMAT_DECIMALS) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  buffer[0] = '\0';

  result = ezo_append_cstr(buffer, buffer_len, &used, prefix);
  if (result != EZO_OK) {
    return result;
  }

  if (value < 0.0) {
    is_negative = 1;
    absolute_value = -value;
  }

  for (i = 0; i < decimals; ++i) {
    scale *= 10U;
  }

  if (absolute_value > ((double)UINT64_MAX / (double)scale)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  scaled_value = (absolute_value * (double)scale) + 0.5;
  scaled_integer = (uint64_t)scaled_value;
  integer_part = scaled_integer / scale;
  fractional_part = scaled_integer % scale;

  if (is_negative) {
    result = ezo_append_char(buffer, buffer_len, &used, '-');
    if (result != EZO_OK) {
      return result;
    }
  }

  result = ezo_append_uint64(buffer, buffer_len, &used, integer_part, 1);
  if (result != EZO_OK) {
    return result;
  }

  if (decimals == 0) {
    return EZO_OK;
  }

  result = ezo_append_char(buffer, buffer_len, &used, '.');
  if (result != EZO_OK) {
    return result;
  }

  return ezo_append_uint64(buffer, buffer_len, &used, fractional_part, decimals);
}

static ezo_result_t ezo_validate_device(const ezo_i2c_device_t *device) {
  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (device->transport == NULL || device->transport->write_then_read == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return EZO_OK;
}

static ezo_result_t ezo_validate_command_kind(ezo_command_kind_t kind) {
  switch (kind) {
  case EZO_COMMAND_GENERIC:
  case EZO_COMMAND_READ:
  case EZO_COMMAND_READ_WITH_TEMP_COMP:
  case EZO_COMMAND_CALIBRATION:
    return EZO_OK;
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }
}

ezo_result_t ezo_device_init(ezo_i2c_device_t *device,
                             uint8_t address,
                             const ezo_i2c_transport_t *transport,
                             void *transport_context) {
  if (device == NULL || transport == NULL || transport->write_then_read == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  device->address = address;
  device->transport = transport;
  device->transport_context = transport_context;
  device->last_device_status = (uint8_t)EZO_STATUS_UNKNOWN;
  return EZO_OK;
}

void ezo_device_set_address(ezo_i2c_device_t *device, uint8_t address) {
  if (device != NULL) {
    device->address = address;
  }
}

uint8_t ezo_device_get_address(const ezo_i2c_device_t *device) {
  if (device == NULL) {
    return 0;
  }

  return device->address;
}

ezo_device_status_t ezo_device_get_last_status(const ezo_i2c_device_t *device) {
  if (device == NULL) {
    return EZO_STATUS_UNKNOWN;
  }

  return (ezo_device_status_t)device->last_device_status;
}

ezo_result_t ezo_get_timing_hint_for_command_kind(ezo_command_kind_t kind,
                                                  ezo_timing_hint_t *timing_hint) {
  uint32_t wait_ms = 0;

  if (timing_hint == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  switch (kind) {
  case EZO_COMMAND_GENERIC:
    wait_ms = 300;
    break;
  case EZO_COMMAND_READ:
  case EZO_COMMAND_READ_WITH_TEMP_COMP:
    wait_ms = 1000;
    break;
  case EZO_COMMAND_CALIBRATION:
    wait_ms = 1200;
    break;
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }

  timing_hint->wait_ms = wait_ms;
  return EZO_OK;
}

ezo_result_t ezo_send_command(ezo_i2c_device_t *device,
                              const char *command,
                              ezo_command_kind_t kind,
                              ezo_timing_hint_t *timing_hint) {
  size_t command_len = 0;
  ezo_result_t result = EZO_OK;
  size_t rx_received = 0;

  result = ezo_validate_device(device);
  if (result != EZO_OK) {
    return result;
  }

  if (command == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_validate_command_kind(kind);
  if (result != EZO_OK) {
    return result;
  }

  command_len = strlen(command);
  if (command_len == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (timing_hint != NULL) {
    result = ezo_get_timing_hint_for_command_kind(kind, timing_hint);
    if (result != EZO_OK) {
      return result;
    }
  }

  return device->transport->write_then_read(device->transport_context,
                                            device->address,
                                            (const uint8_t *)command,
                                            command_len,
                                            NULL,
                                            0,
                                            &rx_received);
}

ezo_result_t ezo_send_command_with_float(ezo_i2c_device_t *device,
                                         const char *prefix,
                                         double value,
                                         uint8_t decimals,
                                         ezo_command_kind_t kind,
                                         ezo_timing_hint_t *timing_hint) {
  char command[64];
  ezo_result_t result = EZO_OK;

  if (prefix == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_format_fixed_command(command, sizeof(command), prefix, value, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_send_command(device, command, kind, timing_hint);
}

ezo_result_t ezo_send_read(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint) {
  return ezo_send_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_send_read_with_temp_comp(ezo_i2c_device_t *device,
                                          double temperature_c,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint) {
  return ezo_send_command_with_float(device,
                                     "rt,",
                                     temperature_c,
                                     decimals,
                                     EZO_COMMAND_READ_WITH_TEMP_COMP,
                                     timing_hint);
}

ezo_result_t ezo_read_response(ezo_i2c_device_t *device,
                               char *buffer,
                               size_t buffer_len,
                               size_t *response_len,
                               ezo_device_status_t *device_status) {
  size_t raw_len = 0;
  size_t rx_received = 0;
  ezo_result_t result = EZO_OK;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  size_t payload_available = 0;
  size_t payload_len = 0;
  size_t i = 0;

  result = ezo_validate_device(device);
  if (result != EZO_OK) {
    return result;
  }

  if (buffer == NULL || buffer_len == 0 || response_len == NULL || device_status == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (buffer_len > EZO_I2C_MAX_TEXT_RESPONSE_LEN) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  raw_len = buffer_len + 1;
  {
    uint8_t raw[EZO_I2C_MAX_RAW_RESPONSE_LEN];

    memset(raw, 0, raw_len);
    memset(buffer, 0, buffer_len);

    result = device->transport->write_then_read(device->transport_context,
                                                device->address,
                                                NULL,
                                                0,
                                                raw,
                                                raw_len,
                                                &rx_received);
    if (result != EZO_OK) {
      device->last_device_status = (uint8_t)EZO_STATUS_UNKNOWN;
      *device_status = EZO_STATUS_UNKNOWN;
      *response_len = 0;
      return result;
    }

    if (rx_received == 0) {
      device->last_device_status = (uint8_t)EZO_STATUS_UNKNOWN;
      *device_status = EZO_STATUS_UNKNOWN;
      *response_len = 0;
      return EZO_ERR_PROTOCOL;
    }

    status = ezo_map_status_byte(raw[0]);
    device->last_device_status = (uint8_t)status;
    *device_status = status;

    if (status == EZO_STATUS_UNKNOWN) {
      *response_len = 0;
      return EZO_ERR_PROTOCOL;
    }

    if (rx_received > 1) {
      payload_available = rx_received - 1;
    }

    for (i = 0; i < payload_available; ++i) {
      if (raw[i + 1] == 0) {
        break;
      }
    }
    payload_len = i;

    if (payload_len >= buffer_len) {
      *response_len = 0;
      return EZO_ERR_BUFFER_TOO_SMALL;
    }

    if (payload_len > 0) {
      memcpy(buffer, &raw[1], payload_len);
    }

    buffer[payload_len] = '\0';

    *response_len = payload_len;
  }

  return EZO_OK;
}

ezo_result_t ezo_parse_double(const char *buffer, size_t buffer_len, double *value_out) {
  size_t i = 0;
  int sign = 1;
  int saw_digit = 0;
  double value = 0.0;
  double fractional_scale = 0.1;

  if (buffer == NULL || value_out == NULL || buffer_len == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  while (i < buffer_len && isspace((unsigned char)buffer[i])) {
    i += 1;
  }

  if (i < buffer_len && (buffer[i] == '+' || buffer[i] == '-')) {
    if (buffer[i] == '-') {
      sign = -1;
    }
    i += 1;
  }

  while (i < buffer_len && isdigit((unsigned char)buffer[i])) {
    saw_digit = 1;
    value = (value * 10.0) + (double)(buffer[i] - '0');
    i += 1;
  }

  if (i < buffer_len && buffer[i] == '.') {
    i += 1;
    while (i < buffer_len && isdigit((unsigned char)buffer[i])) {
      saw_digit = 1;
      value += (double)(buffer[i] - '0') * fractional_scale;
      fractional_scale *= 0.1;
      i += 1;
    }
  }

  if (!saw_digit) {
    return EZO_ERR_PARSE;
  }

  while (i < buffer_len) {
    if (!isspace((unsigned char)buffer[i])) {
      return EZO_ERR_PARSE;
    }
    i += 1;
  }

  if (sign < 0) {
    value = -value;
  }

  *value_out = value;
  return EZO_OK;
}
