#include "ezo_i2c/ezo_i2c.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  int written = 0;

  if (prefix == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  written = snprintf(command, sizeof(command), "%s%.*f", prefix, (int)decimals, value);
  if (written < 0) {
    return EZO_ERR_PROTOCOL;
  }

  if ((size_t)written >= sizeof(command)) {
    return EZO_ERR_BUFFER_TOO_SMALL;
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

  if (buffer_len == SIZE_MAX) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  raw_len = buffer_len + 1;
  {
    uint8_t raw[raw_len];

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

    if (payload_len > buffer_len) {
      *response_len = 0;
      return EZO_ERR_BUFFER_TOO_SMALL;
    }

    if (payload_len > 0) {
      memcpy(buffer, &raw[1], payload_len);
    }

    if (payload_len < buffer_len) {
      buffer[payload_len] = '\0';
    }

    *response_len = payload_len;
  }

  return EZO_OK;
}

ezo_result_t ezo_parse_double(const char *buffer, size_t buffer_len, double *value_out) {
  char *endptr = NULL;
  double parsed = 0.0;
  size_t i = 0;

  if (buffer == NULL || value_out == NULL || buffer_len == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  {
    char local[buffer_len + 1];

    memcpy(local, buffer, buffer_len);
    local[buffer_len] = '\0';

    errno = 0;
    parsed = strtod(local, &endptr);
    if (endptr == local || errno != 0) {
      return EZO_ERR_PARSE;
    }

    for (i = (size_t)(endptr - local); i < buffer_len; ++i) {
      if (!isspace((unsigned char)local[i])) {
        return EZO_ERR_PARSE;
      }
    }
  }

  *value_out = parsed;
  return EZO_OK;
}
