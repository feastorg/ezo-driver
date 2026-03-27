#ifndef BINDINGS_PYTHON_TESTSUPPORT_EZO_TEST_BRIDGE_H
#define BINDINGS_PYTHON_TESTSUPPORT_EZO_TEST_BRIDGE_H

#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  ezo_fake_i2c_transport_t transport;
  ezo_i2c_device_t core;
} ezo_py_fake_i2c_device_t;

typedef struct {
  ezo_fake_uart_transport_t transport;
  ezo_uart_device_t core;
} ezo_py_fake_uart_device_t;

ezo_result_t ezo_py_fake_i2c_device_init(ezo_py_fake_i2c_device_t *device, uint8_t address);
void ezo_py_fake_i2c_device_set_response(ezo_py_fake_i2c_device_t *device,
                                         const uint8_t *response_bytes,
                                         size_t response_len);
ezo_i2c_device_t *ezo_py_fake_i2c_device_core(ezo_py_fake_i2c_device_t *device);

ezo_result_t ezo_py_fake_uart_device_init(ezo_py_fake_uart_device_t *device);
void ezo_py_fake_uart_device_set_response(ezo_py_fake_uart_device_t *device,
                                          const uint8_t *response_bytes,
                                          size_t response_len);
void ezo_py_fake_uart_device_append_response(ezo_py_fake_uart_device_t *device,
                                             const uint8_t *response_bytes,
                                             size_t response_len);
ezo_uart_device_t *ezo_py_fake_uart_device_core(ezo_py_fake_uart_device_t *device);

#ifdef __cplusplus
}
#endif

#endif
