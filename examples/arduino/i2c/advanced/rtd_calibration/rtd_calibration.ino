/*
Purpose: stage RTD single-point offset calibration.
Defaults: Wire on the board default pins, address 102, and reference temperature 25.0 C.
Assumptions: the attached RTD probe is in a stable reference environment before `APPLY_CHANGES` is enabled.
Next: read ../rtd_workflow/rtd_workflow.ino for scale, logger, and memory state.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_rtd.h>

enum CalibrationStep {
  STEP_STATUS = 0,
  STEP_CALIBRATE,
  STEP_CLEAR
};

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 102U;
static const uint8_t APPLY_CHANGES = 0U;
static const CalibrationStep CALIBRATION_STEP = STEP_STATUS;
static const float REFERENCE_TEMPERATURE_C = 25.0f;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t calibration_done = 0U;

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t scale_status;
  ezo_rtd_calibration_status_t calibration;

  EZO_ARDUINO_CHECK_OK("send_scale_query", ezo_rtd_send_scale_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_scale_query", ezo_rtd_read_scale_i2c(&device, &scale_status));

  EZO_ARDUINO_CHECK_OK("send_calibration_query",
                       ezo_rtd_send_calibration_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_calibration_query",
                       ezo_rtd_read_calibration_status_i2c(&device, &calibration));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("current_scale="));
  Serial.println(ezo_arduino_rtd_scale_name(scale_status.scale));
  Serial.print(F("current_calibrated="));
  Serial.println((unsigned)calibration.calibrated);
  Serial.print(F("planned_reference_temperature_c="));
  Serial.println(REFERENCE_TEMPERATURE_C, 3);
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);

  if (APPLY_CHANGES == 0U) {
    return;
  }

  if (CALIBRATION_STEP == STEP_CLEAR) {
    EZO_ARDUINO_CHECK_OK("send_clear_calibration",
                         ezo_rtd_send_clear_calibration_i2c(&device, &hint));
  } else if (CALIBRATION_STEP == STEP_CALIBRATE) {
    EZO_ARDUINO_CHECK_OK("send_calibration",
                         ezo_rtd_send_calibration_i2c(
                             &device, REFERENCE_TEMPERATURE_C, 2U, &hint));
  }
  if (CALIBRATION_STEP != STEP_STATUS) {
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
