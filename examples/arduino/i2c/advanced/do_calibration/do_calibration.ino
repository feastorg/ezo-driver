/*
Purpose: stage D.O. zero/high calibration with explicit compensation-state inspection.
Defaults: Wire on the board default pins, address 97, status step, and default atmospheric pressure.
Assumptions: compensation values stay at vendor defaults during calibration and the probe is in the correct calibration environment.
Next: read ../do_workflow/do_workflow.ino for temperature, salinity, and pressure compensation state.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_do.h>

enum CalibrationStep {
  STEP_STATUS = 0,
  STEP_ZERO,
  STEP_HIGH,
  STEP_CLEAR
};

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 97U;
static const uint8_t APPLY_CHANGES = 0U;
static const CalibrationStep CALIBRATION_STEP = STEP_STATUS;
static const uint8_t PREVIEW_SAMPLES = 5U;
static const unsigned long PREVIEW_INTERVAL_MS = 1000UL;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t calibration_done = 0U;

static ezo_do_calibration_point_t calibration_point() {
  return CALIBRATION_STEP == STEP_ZERO ? EZO_DO_CALIBRATION_ZERO : EZO_DO_CALIBRATION_ATMOSPHERIC;
}

static void preview_readings() {
  ezo_timing_hint_t hint;
  ezo_do_output_config_t output_config;
  ezo_do_reading_t reading;
  uint8_t index = 0U;

  EZO_ARDUINO_CHECK_OK("send_output_query", ezo_do_send_output_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_output_query", ezo_do_read_output_config_i2c(&device, &output_config));

  for (index = 0U; index < PREVIEW_SAMPLES; ++index) {
    EZO_ARDUINO_CHECK_OK("send_preview_read", ezo_do_send_read_i2c(&device, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_preview_response",
                         ezo_do_read_response_i2c(&device, output_config.enabled_mask, &reading));
    ezo_arduino_print_do_reading(F("preview_"), &reading);
    if (index + 1U < PREVIEW_SAMPLES) {
      delay(PREVIEW_INTERVAL_MS);
    }
  }
}

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_do_temperature_compensation_t temperature;
  ezo_do_salinity_compensation_t salinity;
  ezo_do_pressure_compensation_t pressure;
  ezo_do_calibration_status_t calibration;

  EZO_ARDUINO_CHECK_OK("send_temperature_query",
                       ezo_do_send_temperature_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_temperature_query",
                       ezo_do_read_temperature_i2c(&device, &temperature));

  EZO_ARDUINO_CHECK_OK("send_salinity_query", ezo_do_send_salinity_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_salinity_query", ezo_do_read_salinity_i2c(&device, &salinity));

  EZO_ARDUINO_CHECK_OK("send_pressure_query", ezo_do_send_pressure_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_pressure_query", ezo_do_read_pressure_i2c(&device, &pressure));

  EZO_ARDUINO_CHECK_OK("send_calibration_query",
                       ezo_do_send_calibration_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_calibration_query",
                       ezo_do_read_calibration_status_i2c(&device, &calibration));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("current_temperature_compensation_c="));
  Serial.println(temperature.temperature_c, 3);
  Serial.print(F("current_salinity_value="));
  Serial.println(salinity.value, 3);
  Serial.print(F("current_salinity_unit="));
  Serial.println(ezo_arduino_do_salinity_unit_name(salinity.unit));
  Serial.print(F("current_pressure_kpa="));
  Serial.println(pressure.pressure_kpa, 3);
  Serial.print(F("current_calibration_level="));
  Serial.println((unsigned long)calibration.level);
  Serial.println(F("vendor_guidance_calibrate_before_compensating=1"));
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);

  if (CALIBRATION_STEP == STEP_ZERO || CALIBRATION_STEP == STEP_HIGH) {
    preview_readings();
  }

  if (APPLY_CHANGES == 0U) {
    return;
  }

  if (CALIBRATION_STEP == STEP_CLEAR) {
    EZO_ARDUINO_CHECK_OK("send_clear_calibration",
                         ezo_do_send_clear_calibration_i2c(&device, &hint));
  } else if (CALIBRATION_STEP != STEP_STATUS) {
    EZO_ARDUINO_CHECK_OK("send_calibration",
                         ezo_do_send_calibration_i2c(&device, calibration_point(), &hint));
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
