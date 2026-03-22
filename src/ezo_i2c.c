#include "ezo_common.h"
#include "ezo_i2c.h"

#include <string.h>

enum {
  EZO_I2C_MAX_TEXT_FRAME_LEN = EZO_I2C_MAX_RESPONSE_PAYLOAD_LEN + 1,
  EZO_I2C_MAX_RAW_FRAME_LEN = EZO_I2C_MAX_RESPONSE_PAYLOAD_LEN + 2
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

static ezo_result_t ezo_read_response_frame(ezo_i2c_device_t *device,
                                            uint8_t *frame,
                                            size_t frame_len,
                                            size_t *rx_received,
                                            ezo_device_status_t *device_status) {
  ezo_result_t result = EZO_OK;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;

  if (frame == NULL || frame_len == 0 || rx_received == NULL || device_status == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = device->transport->write_then_read(device->transport_context,
                                              device->address,
                                              NULL,
                                              0,
                                              frame,
                                              frame_len,
                                              rx_received);
  if (result != EZO_OK) {
    device->last_device_status = (uint8_t)EZO_STATUS_UNKNOWN;
    *device_status = EZO_STATUS_UNKNOWN;
    *rx_received = 0;
    return result;
  }

  if (*rx_received == 0) {
    device->last_device_status = (uint8_t)EZO_STATUS_UNKNOWN;
    *device_status = EZO_STATUS_UNKNOWN;
    return EZO_ERR_PROTOCOL;
  }

  status = ezo_map_status_byte(frame[0]);
  device->last_device_status = (uint8_t)status;
  *device_status = status;

  if (status == EZO_STATUS_UNKNOWN) {
    return EZO_ERR_PROTOCOL;
  }

  return EZO_OK;
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

const char *ezo_device_status_name(ezo_device_status_t status) {
  switch (status) {
    case EZO_STATUS_UNKNOWN:
      return "EZO_STATUS_UNKNOWN";
    case EZO_STATUS_SUCCESS:
      return "EZO_STATUS_SUCCESS";
    case EZO_STATUS_FAIL:
      return "EZO_STATUS_FAIL";
    case EZO_STATUS_NOT_READY:
      return "EZO_STATUS_NOT_READY";
    case EZO_STATUS_NO_DATA:
      return "EZO_STATUS_NO_DATA";
    default:
      return "EZO_STATUS_INVALID";
  }
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

  device->last_device_status = (uint8_t)EZO_STATUS_UNKNOWN;

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

  result = ezo_common_format_fixed_command(command, sizeof(command), prefix, value, decimals);
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

ezo_result_t ezo_read_response_raw(ezo_i2c_device_t *device,
                                   uint8_t *buffer,
                                   size_t buffer_len,
                                   size_t *response_len,
                                   ezo_device_status_t *device_status) {
  uint8_t frame[EZO_I2C_MAX_RAW_FRAME_LEN];
  size_t frame_len = 0;
  size_t rx_received = 0;
  size_t payload_len = 0;
  ezo_result_t result = EZO_OK;

  result = ezo_validate_device(device);
  if (result != EZO_OK) {
    return result;
  }

  if (buffer == NULL || response_len == NULL || device_status == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (buffer_len == 0 || buffer_len > EZO_I2C_MAX_RESPONSE_PAYLOAD_LEN) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  memset(frame, 0, sizeof(frame));
  memset(buffer, 0, buffer_len);
  *response_len = 0;

  frame_len = buffer_len + 2;
  if (frame_len > sizeof(frame)) {
    frame_len = sizeof(frame);
  }

  result = ezo_read_response_frame(device, frame, frame_len, &rx_received, device_status);
  if (result != EZO_OK) {
    return result;
  }

  if (rx_received > 1) {
    payload_len = rx_received - 1;
  }

  if (payload_len > buffer_len) {
    return EZO_ERR_BUFFER_TOO_SMALL;
  }

  if (payload_len > 0) {
    memcpy(buffer, &frame[1], payload_len);
  }

  *response_len = payload_len;
  return EZO_OK;
}

ezo_result_t ezo_read_response(ezo_i2c_device_t *device,
                               char *buffer,
                               size_t buffer_len,
                               size_t *response_len,
                               ezo_device_status_t *device_status) {
  uint8_t frame[EZO_I2C_MAX_TEXT_FRAME_LEN];
  size_t frame_len = 0;
  size_t rx_received = 0;
  ezo_result_t result = EZO_OK;
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

  frame_len = buffer_len + 1;
  memset(frame, 0, frame_len);
  memset(buffer, 0, buffer_len);
  *response_len = 0;

  result = ezo_read_response_frame(device, frame, frame_len, &rx_received, device_status);
  if (result != EZO_OK) {
    return result;
  }

  if (rx_received > 1) {
    payload_available = rx_received - 1;
  }

  for (i = 0; i < payload_available; ++i) {
    if (frame[i + 1] == 0) {
      break;
    }
  }
  payload_len = i;

  if (payload_len >= buffer_len) {
    return EZO_ERR_BUFFER_TOO_SMALL;
  }

  if (payload_len > 0) {
    memcpy(buffer, &frame[1], payload_len);
  }

  buffer[payload_len] = '\0';
  *response_len = payload_len;
  return EZO_OK;
}
