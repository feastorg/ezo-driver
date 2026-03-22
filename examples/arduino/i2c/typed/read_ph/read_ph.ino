/*
Purpose: simple Arduino I2C pH read using the C++ wrapper.
Defaults: Wire on the board default pins and address 99.
Assumptions: the device is a pH circuit already in I2C mode.
Next: read ../../commissioning/inspect_device/inspect_device.ino for setup checks.
*/

#include <Wire.h>

#include <ezo_i2c.hpp>
#include <ezo_i2c_arduino_wire.h>

enum {
  STARTUP_SETTLE_MS = 1000U,
  PH_I2C_ADDRESS = 99U
};

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c::Device ph_device;
static unsigned long startup_started_at_ms = 0;
static uint8_t read_requested = 0;

static void fail_fast(const char *step, ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

  Serial.print("driver_error_step=");
  Serial.println(step);
  Serial.print("driver_error_name=");
  Serial.println(ezo_result_name(result));
  Serial.print("driver_error_code=");
  Serial.println((int)result);
  while (true) {
  }
}

#define CHECK_OK(step, expr) fail_fast(step, (expr))

static void request_reading() {
  ezo_i2c::TimingHint hint;
  CHECK_OK("send_read", ph_device.send_read(&hint));
  delay(hint.wait_ms);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  CHECK_OK("init_wire_context", ezo_arduino_wire_context_init(&wire_context, &Wire));
  CHECK_OK("init_device", ph_device.init(PH_I2C_ADDRESS, ezo_arduino_wire_transport(), &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  char response[32];
  size_t response_len = 0;
  ezo_i2c::DeviceStatus status = EZO_STATUS_UNKNOWN;
  double value = 0.0;

  if (read_requested == 0U) {
    if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
      return;
    }

    request_reading();
    read_requested = 1U;
    return;
  }

  CHECK_OK("read_response", ph_device.read_response(response, sizeof(response), &response_len, &status));

  if (status == EZO_STATUS_SUCCESS &&
      ph_device.parse_double(response, response_len, &value) == EZO_OK) {
    Serial.print("ph=");
    Serial.println(value, 3);
  } else {
    Serial.print("device_status_name=");
    Serial.println(ezo_device_status_name(status));
    Serial.print("device_status_code=");
    Serial.println((int)status);
  }

  delay(1000);
  request_reading();
}
