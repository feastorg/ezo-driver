/*
Purpose: simple typed Arduino I2C pH read.
Defaults: Wire on the board default pins and address 99.
Assumptions: the device is a pH circuit already in I2C mode.
Next: read ../../commissioning/inspect_device/inspect_device.ino for setup checks.
*/

#include <Wire.h>

#include <ezo_i2c_arduino_wire.h>
#include <ezo_ph.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const uint8_t PH_I2C_ADDRESS = 99U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
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
  ezo_timing_hint_t hint;

  CHECK_OK("send_read", ezo_ph_send_read_i2c(&device, &hint));
  delay(hint.wait_ms);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  CHECK_OK("init_wire_context", ezo_arduino_wire_context_init(&wire_context, &Wire));
  CHECK_OK("init_device",
           ezo_device_init(&device, PH_I2C_ADDRESS, ezo_arduino_wire_transport(), &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_ph_reading_t reading;

  if (read_requested == 0U) {
    if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
      return;
    }

    request_reading();
    read_requested = 1U;
    return;
  }

  CHECK_OK("read_response", ezo_ph_read_response_i2c(&device, &reading));

  Serial.print("ph=");
  Serial.println(reading.ph, 3);

  delay(1000);
  request_reading();
}
