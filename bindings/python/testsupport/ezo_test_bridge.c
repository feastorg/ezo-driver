#include "ezo_test_bridge.h"

ezo_result_t ezo_py_fake_i2c_device_init(ezo_py_fake_i2c_device_t *device, uint8_t address) {
  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  ezo_fake_i2c_transport_init(&device->transport);
  return ezo_device_init(&device->core, address, ezo_fake_i2c_transport_vtable(), &device->transport);
}

void ezo_py_fake_i2c_device_set_response(ezo_py_fake_i2c_device_t *device,
                                         const uint8_t *response_bytes,
                                         size_t response_len) {
  if (device == NULL) {
    return;
  }

  ezo_fake_i2c_transport_set_response(&device->transport, response_bytes, response_len);
}

ezo_i2c_device_t *ezo_py_fake_i2c_device_core(ezo_py_fake_i2c_device_t *device) {
  if (device == NULL) {
    return 0;
  }
  return &device->core;
}

ezo_result_t ezo_py_fake_uart_device_init(ezo_py_fake_uart_device_t *device) {
  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  ezo_fake_uart_transport_init(&device->transport);
  return ezo_uart_device_init(&device->core, ezo_fake_uart_transport_vtable(), &device->transport);
}

void ezo_py_fake_uart_device_set_response(ezo_py_fake_uart_device_t *device,
                                          const uint8_t *response_bytes,
                                          size_t response_len) {
  if (device == NULL) {
    return;
  }

  ezo_fake_uart_transport_set_response(&device->transport, response_bytes, response_len);
}

void ezo_py_fake_uart_device_append_response(ezo_py_fake_uart_device_t *device,
                                             const uint8_t *response_bytes,
                                             size_t response_len) {
  if (device == NULL) {
    return;
  }

  ezo_fake_uart_transport_append_response(&device->transport, response_bytes, response_len);
}

ezo_uart_device_t *ezo_py_fake_uart_device_core(ezo_py_fake_uart_device_t *device) {
  if (device == NULL) {
    return 0;
  }
  return &device->core;
}
