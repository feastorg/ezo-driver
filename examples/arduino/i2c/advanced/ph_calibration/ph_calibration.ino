/*
Purpose: stage pH calibration steps with bounded preview reads and explicit vendor ordering notes.
Defaults: Wire on the board default pins, address 99, midpoint step, and reference 7.00.
Assumptions: the probe is in the matching calibration solution before `APPLY_CHANGES` is enabled.
Next: read ../ph_workflow/ph_workflow.ino for operational temperature-compensation and range state.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_ph.h>

enum CalibrationStep {
  STEP_STATUS = 0,
  STEP_MID,
  STEP_LOW,
  STEP_HIGH,
  STEP_CLEAR
};

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 99U;
static const uint8_t APPLY_CHANGES = 0U;
static const CalibrationStep CALIBRATION_STEP = STEP_STATUS;
static const float REFERENCE_PH = 7.00f;
static const uint8_t PREVIEW_SAMPLES = 5U;
static const unsigned long PREVIEW_INTERVAL_MS = 1000UL;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t calibration_done = 0U;

static ezo_ph_calibration_point_t calibration_point() {
  switch (CALIBRATION_STEP) {
    case STEP_LOW:
      return EZO_PH_CALIBRATION_POINT_LOW;
    case STEP_HIGH:
      return EZO_PH_CALIBRATION_POINT_HIGH;
    case STEP_MID:
    case STEP_CLEAR:
    case STEP_STATUS:
    default:
      return EZO_PH_CALIBRATION_POINT_MID;
  }
}

static const __FlashStringHelper *step_name() {
  switch (CALIBRATION_STEP) {
    case STEP_MID:
      return F("mid");
    case STEP_LOW:
      return F("low");
    case STEP_HIGH:
      return F("high");
    case STEP_CLEAR:
      return F("clear");
    case STEP_STATUS:
    default:
      return F("status");
  }
}

static void preview_readings() {
  ezo_timing_hint_t hint;
  ezo_ph_reading_t reading;
  uint8_t index = 0U;

  for (index = 0U; index < PREVIEW_SAMPLES; ++index) {
    EZO_ARDUINO_CHECK_OK("send_preview_read", ezo_ph_send_read_i2c(&device, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_preview_response", ezo_ph_read_response_i2c(&device, &reading));
    Serial.print(F("preview_reading_"));
    Serial.print((unsigned)index);
    Serial.print(F("_ph="));
    Serial.println(reading.ph, 3);
    if (index + 1U < PREVIEW_SAMPLES) {
      delay(PREVIEW_INTERVAL_MS);
    }
  }
}

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_ph_temperature_compensation_t temperature;
  ezo_ph_calibration_status_t calibration;
  ezo_ph_slope_t slope;

  EZO_ARDUINO_CHECK_OK("send_temperature_query", ezo_ph_send_temperature_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_temperature_query", ezo_ph_read_temperature_i2c(&device, &temperature));

  EZO_ARDUINO_CHECK_OK("send_calibration_query", ezo_ph_send_calibration_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_calibration_query",
                       ezo_ph_read_calibration_status_i2c(&device, &calibration));

  EZO_ARDUINO_CHECK_OK("send_slope_query", ezo_ph_send_slope_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_slope_query", ezo_ph_read_slope_i2c(&device, &slope));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("step="));
  Serial.println(step_name());
  Serial.print(F("current_temperature_compensation_c="));
  Serial.println(temperature.temperature_c, 3);
  Serial.print(F("current_calibration_level="));
  Serial.println(ezo_arduino_ph_calibration_name(calibration.level));
  Serial.print(F("current_slope_acid_percent="));
  Serial.println(slope.acid_percent, 3);
  Serial.print(F("current_slope_base_percent="));
  Serial.println(slope.base_percent, 3);
  Serial.print(F("current_slope_neutral_mv="));
  Serial.println(slope.neutral_mv, 3);
  Serial.println(F("vendor_guidance_midpoint_first=1"));
  Serial.println(F("vendor_guidance_midpoint_clears_other_points=1"));
  Serial.println(F("vendor_guidance_check_slope_after_calibration=1"));
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);
  Serial.print(F("planned_reference_ph="));
  Serial.println(REFERENCE_PH, 3);

  if (CALIBRATION_STEP != STEP_STATUS && CALIBRATION_STEP != STEP_CLEAR) {
    preview_readings();
  }

  if (APPLY_CHANGES == 0U) {
    return;
  }

  if (CALIBRATION_STEP == STEP_CLEAR) {
    EZO_ARDUINO_CHECK_OK("send_clear_calibration",
                         ezo_ph_send_clear_calibration_i2c(&device, &hint));
  } else if (CALIBRATION_STEP != STEP_STATUS) {
    EZO_ARDUINO_CHECK_OK("send_calibration",
                         ezo_ph_send_calibration_i2c(
                             &device, calibration_point(), REFERENCE_PH, 2U, &hint));
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
