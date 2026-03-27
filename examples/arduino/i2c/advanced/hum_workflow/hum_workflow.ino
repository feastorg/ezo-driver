/*
Purpose: inspect HUM output selection, temperature-calibration state, and current reading, with optional output and Tcal updates.
Defaults: Wire on the board default pins, address 111, and reference temperature 25.0 C.
Assumptions: the device is a HUM circuit already in I2C mode.
Next: read ../hum_temperature_calibration/hum_temperature_calibration.ino for staged temperature calibration.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_hum.h>

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 111U;
static const uint8_t APPLY_CHANGES = 0U;
static const uint8_t ENABLE_DEW_POINT_OUTPUT = 1U;
static const float PLANNED_REFERENCE_TEMPERATURE_C = 25.0f;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t workflow_done = 0U;

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_hum_output_config_t output_config;
  ezo_hum_temperature_calibration_status_t calibration;
  ezo_hum_reading_t reading;

  EZO_ARDUINO_CHECK_OK("send_output_query", ezo_hum_send_output_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_output_query", ezo_hum_read_output_config_i2c(&device, &output_config));

  EZO_ARDUINO_CHECK_OK("send_temperature_calibration_query",
                       ezo_hum_send_temperature_calibration_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_temperature_calibration_query",
                       ezo_hum_read_temperature_calibration_status_i2c(&device, &calibration));

  EZO_ARDUINO_CHECK_OK("send_read", ezo_hum_send_read_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_response",
                       ezo_hum_read_response_i2c(&device, output_config.enabled_mask, &reading));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("current_output_mask="));
  Serial.println((unsigned long)output_config.enabled_mask);
  Serial.print(F("current_temperature_calibrated="));
  Serial.println((unsigned)calibration.calibrated);
  ezo_arduino_print_hum_reading(F("current_"), &reading);
  Serial.println(F("vendor_note_humidity_factory_calibrated=1"));
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);

  if (APPLY_CHANGES == 0U) {
    return;
  }

  EZO_ARDUINO_CHECK_OK("send_output_set",
                       ezo_hum_send_output_set_i2c(
                           &device, EZO_HUM_OUTPUT_DEW_POINT, ENABLE_DEW_POINT_OUTPUT, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_temperature_calibration",
                       ezo_hum_send_temperature_calibration_i2c(
                           &device, PLANNED_REFERENCE_TEMPERATURE_C, 2U, &hint));
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
