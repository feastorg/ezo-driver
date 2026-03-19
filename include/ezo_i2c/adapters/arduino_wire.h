#ifndef EZO_I2C_ADAPTERS_ARDUINO_WIRE_H
#define EZO_I2C_ADAPTERS_ARDUINO_WIRE_H

#ifndef __cplusplus
#error "arduino_wire adapter requires C++"
#endif

#include <Wire.h>

#include "ezo_i2c/ezo_i2c.h"

struct ezo_arduino_wire_context_t {
  TwoWire *wire;
};

ezo_result_t ezo_arduino_wire_context_init(ezo_arduino_wire_context_t *context, TwoWire *wire);
const ezo_i2c_transport_t *ezo_arduino_wire_transport(void);

#endif
