/*
Purpose: simple Arduino I2C ORP read.
Defaults: Wire on the board default pins and address 98.
Assumptions: the device is an ORP circuit already in I2C mode.
Next: read ../../advanced/orp_workflow/orp_workflow.ino for calibration and extended-scale state.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_orp.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const uint8_t ORP_I2C_ADDRESS = 98U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t read_requested = 0U;

static void request_reading() {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_read", ezo_orp_send_read_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_device",
                       ezo_arduino_i2c_init_device(&device, ORP_I2C_ADDRESS, &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_orp_reading_t reading;

  if (read_requested == 0U) {
    if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
      return;
    }

    request_reading();
    read_requested = 1U;
    return;
  }

  EZO_ARDUINO_CHECK_OK("read_response", ezo_orp_read_response_i2c(&device, &reading));
  Serial.print(F("millivolts="));
  Serial.println(reading.millivolts, 3);

  delay(1000);
  request_reading();
}
