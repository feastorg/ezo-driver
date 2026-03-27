/*
Purpose: stage ORP single-point calibration with bounded preview reads.
Defaults: Wire on the board default pins, address 98, and reference 225 mV.
Assumptions: the probe is in the matching ORP calibration solution before `APPLY_CHANGES` is enabled.
Next: read ../orp_workflow/orp_workflow.ino for extended-scale state and ongoing reads.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_orp.h>

enum CalibrationStep {
  STEP_STATUS = 0,
  STEP_CALIBRATE,
  STEP_CLEAR
};

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 98U;
static const uint8_t APPLY_CHANGES = 0U;
static const CalibrationStep CALIBRATION_STEP = STEP_STATUS;
static const float REFERENCE_MV = 225.0f;
static const uint8_t PREVIEW_SAMPLES = 5U;
static const unsigned long PREVIEW_INTERVAL_MS = 1000UL;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t calibration_done = 0U;

static void preview_readings() {
  ezo_timing_hint_t hint;
  ezo_orp_reading_t reading;
  uint8_t index = 0U;

  for (index = 0U; index < PREVIEW_SAMPLES; ++index) {
    EZO_ARDUINO_CHECK_OK("send_preview_read", ezo_orp_send_read_i2c(&device, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_preview_response", ezo_orp_read_response_i2c(&device, &reading));
    Serial.print(F("preview_reading_"));
    Serial.print((unsigned)index);
    Serial.print(F("_mv="));
    Serial.println(reading.millivolts, 3);
    if (index + 1U < PREVIEW_SAMPLES) {
      delay(PREVIEW_INTERVAL_MS);
    }
  }
}

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_orp_calibration_status_t calibration;

  EZO_ARDUINO_CHECK_OK("send_calibration_query",
                       ezo_orp_send_calibration_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_calibration_query",
                       ezo_orp_read_calibration_status_i2c(&device, &calibration));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("current_calibrated="));
  Serial.println((unsigned)calibration.calibrated);
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);
  Serial.print(F("planned_reference_mv="));
  Serial.println(REFERENCE_MV, 3);

  if (CALIBRATION_STEP == STEP_CALIBRATE) {
    preview_readings();
  }

  if (APPLY_CHANGES == 0U) {
    return;
  }

  if (CALIBRATION_STEP == STEP_CLEAR) {
    EZO_ARDUINO_CHECK_OK("send_clear_calibration",
                         ezo_orp_send_clear_calibration_i2c(&device, &hint));
    ezo_arduino_wait_hint(&hint);
  } else if (CALIBRATION_STEP == STEP_CALIBRATE) {
    EZO_ARDUINO_CHECK_OK("send_calibration",
                         ezo_orp_send_calibration_i2c(&device, REFERENCE_MV, 0U, &hint));
    ezo_arduino_wait_hint(&hint);
  }
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_device",
                       ezo_arduino_i2c_init_device(&device, DEVICE_I2C_ADDRESS, &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  if (calibration_done != 0U) {
    return;
  }

  if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
    return;
  }

  run_workflow();
  calibration_done = 1U;
}
