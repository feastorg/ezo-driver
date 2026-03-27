/*
Purpose: inspect ORP calibration and extended-scale state, with optional extended-scale changes.
Defaults: Wire on the board default pins and address 98.
Assumptions: the device is an ORP circuit already in I2C mode.
Next: read ../orp_calibration/orp_calibration.ino for staged single-point calibration.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_orp.h>

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 98U;
static const uint8_t APPLY_CHANGES = 0U;
static const uint8_t ENABLE_EXTENDED_SCALE = 0U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t workflow_done = 0U;

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_orp_reading_t reading;
  ezo_orp_calibration_status_t calibration;
  ezo_orp_extended_scale_status_t range_status;

  EZO_ARDUINO_CHECK_OK("send_read", ezo_orp_send_read_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_response", ezo_orp_read_response_i2c(&device, &reading));

  EZO_ARDUINO_CHECK_OK("send_calibration_query",
                       ezo_orp_send_calibration_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_calibration_query",
                       ezo_orp_read_calibration_status_i2c(&device, &calibration));

  EZO_ARDUINO_CHECK_OK("send_extended_scale_query",
                       ezo_orp_send_extended_scale_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_extended_scale_query",
                       ezo_orp_read_extended_scale_i2c(&device, &range_status));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("current_millivolts="));
  Serial.println(reading.millivolts, 3);
  Serial.print(F("current_calibrated="));
  Serial.println((unsigned)calibration.calibrated);
  Serial.print(F("current_extended_scale_enabled="));
  Serial.println((unsigned)range_status.enabled);
  Serial.println(F("vendor_note_extended_scale_requires_5v=1"));
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);

  if (APPLY_CHANGES == 0U) {
    return;
  }

  EZO_ARDUINO_CHECK_OK("send_extended_scale_set",
                       ezo_orp_send_extended_scale_set_i2c(
                           &device,
                           ENABLE_EXTENDED_SCALE != 0U ? EZO_ORP_EXTENDED_SCALE_ENABLED
                                                       : EZO_ORP_EXTENDED_SCALE_DISABLED,
                           &hint));
  ezo_arduino_wait_hint(&hint);
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_device",
                       ezo_arduino_i2c_init_device(&device, DEVICE_I2C_ADDRESS, &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  if (workflow_done != 0U) {
    return;
  }

  if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
    return;
  }

  run_workflow();
  workflow_done = 1U;
}
