/*
Purpose: inspect pH temperature-compensation, calibration, slope, and extended-range state, with an optional one-shot temperature-compensated read.
Defaults: Wire on the board default pins, address 99, and planned temperature 25.0 C.
Assumptions: the device is a pH circuit already in I2C mode.
Next: read ../ph_calibration/ph_calibration.ino for staged vendor-order calibration.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_ph.h>

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 99U;
static const uint8_t APPLY_CHANGES = 0U;
static const float PLANNED_TEMPERATURE_C = 25.0f;
static const uint8_t ENABLE_EXTENDED_RANGE = 0U;
static const uint8_t RUN_ONE_SHOT_TEMP_COMP_READ = 1U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t workflow_done = 0U;

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_ph_temperature_compensation_t temperature;
  ezo_ph_calibration_status_t calibration;
  ezo_ph_slope_t slope;
  ezo_ph_extended_range_status_t range_status;
  ezo_ph_reading_t reading;

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

  EZO_ARDUINO_CHECK_OK("send_extended_range_query",
                       ezo_ph_send_extended_range_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_extended_range_query",
                       ezo_ph_read_extended_range_i2c(&device, &range_status));

  EZO_ARDUINO_CHECK_OK("send_read", ezo_ph_send_read_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_response", ezo_ph_read_response_i2c(&device, &reading));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("current_ph="));
  Serial.println(reading.ph, 3);
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
  Serial.print(F("current_extended_range_enabled="));
  Serial.println((unsigned)range_status.enabled);
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);

  if (APPLY_CHANGES == 0U) {
    return;
  }

  if (RUN_ONE_SHOT_TEMP_COMP_READ != 0U) {
    EZO_ARDUINO_CHECK_OK("send_read_with_temp_comp",
                         ezo_ph_send_read_with_temp_comp_i2c(
                             &device, PLANNED_TEMPERATURE_C, 2U, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_response_with_temp_comp", ezo_ph_read_response_i2c(&device, &reading));
    Serial.print(F("one_shot_temp_comp_ph="));
    Serial.println(reading.ph, 3);
  }

  EZO_ARDUINO_CHECK_OK("send_temperature_set",
                       ezo_ph_send_temperature_set_i2c(
                           &device, PLANNED_TEMPERATURE_C, 2U, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_extended_range_set",
                       ezo_ph_send_extended_range_set_i2c(
                           &device,
                           ENABLE_EXTENDED_RANGE != 0U ? EZO_PH_EXTENDED_RANGE_ENABLED
                                                       : EZO_PH_EXTENDED_RANGE_DISABLED,
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
