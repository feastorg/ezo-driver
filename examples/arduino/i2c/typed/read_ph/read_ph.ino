/*
Purpose: simple Arduino I2C pH read using the C++ wrapper.
Defaults: Wire on the board default pins and address 99.
Assumptions: the device is a pH circuit already in I2C mode.
Next: read ../../commissioning/inspect_device/inspect_device.ino for setup checks.
*/

#include <Wire.h>

#include <ezo_i2c.hpp>
#include <ezo_i2c_arduino_wire.h>

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c::Device ph_device;

static void fail_fast(ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

  Serial.print("driver error: ");
  Serial.println((int)result);
  while (true) {
  }
}

void setup() {
  ezo_i2c::TimingHint hint;

  Serial.begin(115200);
  Wire.begin();

  fail_fast(ezo_arduino_wire_context_init(&wire_context, &Wire));
  fail_fast(ph_device.init(99, ezo_arduino_wire_transport(), &wire_context));
  fail_fast(ph_device.send_read(&hint));

  delay(hint.wait_ms);
}

void loop() {
  char response[32];
  size_t response_len = 0;
  ezo_i2c::DeviceStatus status = EZO_STATUS_UNKNOWN;
  double value = 0.0;

  fail_fast(ph_device.read_response(response, sizeof(response), &response_len, &status));

  if (status == EZO_STATUS_SUCCESS &&
      ph_device.parse_double(response, response_len, &value) == EZO_OK) {
    Serial.print("ph=");
    Serial.println(value, 3);
  } else {
    Serial.print("device_status=");
    Serial.println((int)status);
  }

  delay(1000);

  {
    ezo_i2c::TimingHint hint;
    fail_fast(ph_device.send_read(&hint));
    delay(hint.wait_ms);
  }
}
