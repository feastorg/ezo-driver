#ifndef EZO_ARDUINO_I2C_EXAMPLE_HPP
#define EZO_ARDUINO_I2C_EXAMPLE_HPP

#include <Wire.h>

#include <ezo_arduino_common.hpp>
#include <ezo_i2c_arduino_wire.h>

inline void ezo_arduino_i2c_begin() {
  Serial.begin(115200);
  Wire.begin();
}

inline ezo_result_t ezo_arduino_i2c_init_context(ezo_arduino_wire_context_t *context) {
  if (context == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }
  return ezo_arduino_wire_context_init(context, &Wire);
}

inline ezo_result_t ezo_arduino_i2c_init_device(ezo_i2c_device_t *device,
                                                uint8_t address,
                                                ezo_arduino_wire_context_t *context) {
  if (device == NULL || context == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }
  return ezo_device_init(device, address, ezo_arduino_wire_transport(), context);
}

inline void ezo_arduino_i2c_drain_pending(ezo_i2c_device_t *device) {
  char buffer[16];
  size_t response_len = 0U;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  uint8_t attempt = 0U;

  if (device == NULL) {
    return;
  }

  for (attempt = 0U; attempt < 2U; ++attempt) {
    ezo_result_t result =
        ezo_read_response(device, buffer, sizeof(buffer), &response_len, &status);
    if (result != EZO_OK && result != EZO_ERR_BUFFER_TOO_SMALL && result != EZO_ERR_PROTOCOL) {
      ezo_arduino_fail_fast("drain_pending_i2c", result);
    }
    delay(200);
  }
}

#endif
