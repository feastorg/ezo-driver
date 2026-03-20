#include <Wire.h>

#include <ezo_i2c_arduino_wire.h>
#include <ezo_i2c.h>

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;

static void fail_fast(ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

  while (true) {
  }
}

void setup() {
  ezo_timing_hint_t hint;

  Wire.begin();

  fail_fast(ezo_arduino_wire_context_init(&wire_context, &Wire));
  fail_fast(ezo_device_init(&device, 99, ezo_arduino_wire_transport(), &wire_context));
  fail_fast(ezo_send_command(&device, "name,?", EZO_COMMAND_GENERIC, &hint));
}

void loop() {
}
