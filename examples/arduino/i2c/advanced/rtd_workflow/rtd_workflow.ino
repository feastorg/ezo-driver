/*
Purpose: inspect RTD scale, calibration, logger, memory state, and current reading, with optional scale/logger/memory updates.
Defaults: Wire on the board default pins, address 102, planned scale Celsius, and logger interval 0.
Assumptions: logger should be disabled before bulk memory recall or clear operations.
Next: read ../rtd_calibration/rtd_calibration.ino for staged single-point calibration.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_rtd.h>

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 102U;
static const uint8_t APPLY_CHANGES = 0U;
static const ezo_rtd_scale_t PLANNED_SCALE = EZO_RTD_SCALE_CELSIUS;
static const uint32_t PLANNED_LOGGER_INTERVAL_UNITS = 0U;
static const uint8_t CLEAR_MEMORY = 0U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t workflow_done = 0U;

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t scale_status;
  ezo_rtd_calibration_status_t calibration;
  ezo_rtd_logger_status_t logger;
  ezo_rtd_memory_status_t memory_status;
  ezo_rtd_reading_t reading;

  EZO_ARDUINO_CHECK_OK("send_scale_query", ezo_rtd_send_scale_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_scale_query", ezo_rtd_read_scale_i2c(&device, &scale_status));

  EZO_ARDUINO_CHECK_OK("send_calibration_query",
                       ezo_rtd_send_calibration_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_calibration_query",
                       ezo_rtd_read_calibration_status_i2c(&device, &calibration));

  EZO_ARDUINO_CHECK_OK("send_logger_query", ezo_rtd_send_logger_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_logger_query", ezo_rtd_read_logger_i2c(&device, &logger));

  EZO_ARDUINO_CHECK_OK("send_memory_query", ezo_rtd_send_memory_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_memory_query", ezo_rtd_read_memory_status_i2c(&device, &memory_status));

  EZO_ARDUINO_CHECK_OK("send_read", ezo_rtd_send_read_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_response",
                       ezo_rtd_read_response_i2c(&device, scale_status.scale, &reading));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("current_scale="));
  Serial.println(ezo_arduino_rtd_scale_name(scale_status.scale));
  Serial.print(F("current_calibrated="));
  Serial.println((unsigned)calibration.calibrated);
  Serial.print(F("current_logger_interval_units="));
  Serial.println((unsigned long)logger.interval_units);
  Serial.print(F("current_memory_last_index="));
  Serial.println((unsigned long)memory_status.last_index);
  Serial.print(F("current_temperature="));
  Serial.println(reading.temperature, 3);
  Serial.println(F("vendor_guidance_disable_logger_before_recall=1"));
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);

  if (APPLY_CHANGES == 0U) {
    return;
  }

  EZO_ARDUINO_CHECK_OK("send_scale_set", ezo_rtd_send_scale_set_i2c(&device, PLANNED_SCALE, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_logger_set",
                       ezo_rtd_send_logger_set_i2c(&device, PLANNED_LOGGER_INTERVAL_UNITS, &hint));
  ezo_arduino_wait_hint(&hint);
  if (CLEAR_MEMORY != 0U) {
    EZO_ARDUINO_CHECK_OK("send_memory_clear", ezo_rtd_send_memory_clear_i2c(&device, &hint));
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
  if (workflow_done != 0U) {
    return;
  }

  if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
    return;
  }

  run_workflow();
  workflow_done = 1U;
}
