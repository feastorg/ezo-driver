/*
Purpose: minimal Arduino I2C raw smoke path.
Defaults: Wire on the board default pins and address 99.
Assumptions: the device is already in I2C mode and wired to the selected bus.
Next: read ../../commissioning/inspect_device/inspect_device.ino for identity checks.
*/

#include <Wire.h>

#include <ezo_i2c.h>
#include <ezo_i2c_arduino_wire.h>

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

  Serial.begin(115200);
  Wire.begin();

  fail_fast(ezo_arduino_wire_context_init(&wire_context, &Wire));
  fail_fast(ezo_device_init(&device, 99, ezo_arduino_wire_transport(), &wire_context));
  fail_fast(ezo_send_command(&device, "name,?", EZO_COMMAND_GENERIC, &hint));

  Serial.println("smoke_sent=1");
}

void loop() {
}
