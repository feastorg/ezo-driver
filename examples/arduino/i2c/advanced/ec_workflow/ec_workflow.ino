/*
Purpose: inspect EC output selection, temperature compensation, probe-K, TDS-factor, calibration state, and current reading.
Defaults: Wire on the board default pins, address 100, temperature 25.0 C, probe-K 1.0, and TDS factor 0.5.
Assumptions: the device is an EC circuit already in I2C mode.
Next: read ../ec_calibration/ec_calibration.ino for staged dry/single/low/high calibration.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_ec.h>

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 100U;
static const uint8_t APPLY_CHANGES = 0U;
static const uint8_t ENABLE_SALINITY_OUTPUT = 1U;
static const float PLANNED_TEMPERATURE_C = 25.0f;
static const float PLANNED_PROBE_K = 1.0f;
static const float PLANNED_TDS_FACTOR = 0.5f;
static const uint8_t RUN_ONE_SHOT_TEMP_COMP_READ = 1U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t workflow_done = 0U;

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_ec_output_config_t output_config;
  ezo_ec_temperature_compensation_t temperature;
  ezo_ec_probe_k_t probe_k;
  ezo_ec_tds_factor_t tds_factor;
  ezo_ec_calibration_status_t calibration;
  ezo_ec_reading_t reading;

  EZO_ARDUINO_CHECK_OK("send_output_query", ezo_ec_send_output_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_output_query", ezo_ec_read_output_config_i2c(&device, &output_config));

  EZO_ARDUINO_CHECK_OK("send_temperature_query",
                       ezo_ec_send_temperature_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_temperature_query",
                       ezo_ec_read_temperature_i2c(&device, &temperature));

  EZO_ARDUINO_CHECK_OK("send_probe_k_query", ezo_ec_send_probe_k_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_probe_k_query", ezo_ec_read_probe_k_i2c(&device, &probe_k));

  EZO_ARDUINO_CHECK_OK("send_tds_factor_query", ezo_ec_send_tds_factor_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_tds_factor_query", ezo_ec_read_tds_factor_i2c(&device, &tds_factor));

  EZO_ARDUINO_CHECK_OK("send_calibration_query",
                       ezo_ec_send_calibration_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_calibration_query",
                       ezo_ec_read_calibration_status_i2c(&device, &calibration));

  EZO_ARDUINO_CHECK_OK("send_read", ezo_ec_send_read_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_response",
                       ezo_ec_read_response_i2c(&device, output_config.enabled_mask, &reading));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("current_output_mask="));
  Serial.println((unsigned long)output_config.enabled_mask);
  Serial.print(F("current_temperature_compensation_c="));
  Serial.println(temperature.temperature_c, 3);
  Serial.print(F("current_probe_k="));
  Serial.println(probe_k.k_value, 3);
  Serial.print(F("current_tds_factor="));
  Serial.println(tds_factor.factor, 3);
  Serial.print(F("current_calibration_level="));
  Serial.println((unsigned long)calibration.level);
  ezo_arduino_print_ec_reading(F("current_"), &reading);
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);

  if (RUN_ONE_SHOT_TEMP_COMP_READ != 0U) {
    EZO_ARDUINO_CHECK_OK("send_read_with_temp_comp",
                         ezo_ec_send_read_with_temp_comp_i2c(
                             &device, PLANNED_TEMPERATURE_C, 2U, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_response_with_temp_comp",
                         ezo_ec_read_response_i2c(&device, output_config.enabled_mask, &reading));
    ezo_arduino_print_ec_reading(F("one_shot_"), &reading);
  }

  if (APPLY_CHANGES == 0U) {
    return;
  }

  EZO_ARDUINO_CHECK_OK("send_output_set",
                       ezo_ec_send_output_set_i2c(
                           &device, EZO_EC_OUTPUT_SALINITY, ENABLE_SALINITY_OUTPUT, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_temperature_set",
                       ezo_ec_send_temperature_set_i2c(
                           &device, PLANNED_TEMPERATURE_C, 2U, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_probe_k_set",
                       ezo_ec_send_probe_k_set_i2c(&device, PLANNED_PROBE_K, 2U, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_tds_factor_set",
                       ezo_ec_send_tds_factor_set_i2c(&device, PLANNED_TDS_FACTOR, 2U, &hint));
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
