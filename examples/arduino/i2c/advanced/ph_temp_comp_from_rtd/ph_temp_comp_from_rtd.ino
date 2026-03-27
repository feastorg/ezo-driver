/*
Purpose: drive pH temperature compensation from a live RTD reading on Arduino I2C.
Defaults: Wire on the board default pins, RTD at 102, and pH at 99.
Assumptions: both circuits share the same I2C bus and this sketch is allowed to update the pH temperature-compensation state.
Next: read ../ph_workflow/ph_workflow.ino for persistent pH temperature-compensation state inspection.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_ph.h>
#include <ezo_rtd.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const unsigned long POLL_INTERVAL_MS = 2000UL;
static const uint8_t RTD_I2C_ADDRESS = 102U;
static const uint8_t PH_I2C_ADDRESS = 99U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t rtd_device;
static ezo_i2c_device_t ph_device;
static unsigned long startup_started_at_ms = 0;
static unsigned long last_cycle_started_at_ms = 0;

static void run_cycle() {
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_ph_temperature_compensation_t ph_temperature;
  ezo_ph_reading_t ph_reading;
  float source_temperature_c = 0.0f;

  EZO_ARDUINO_CHECK_OK("send_rtd_scale_query", ezo_rtd_send_scale_query_i2c(&rtd_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_rtd_scale_query", ezo_rtd_read_scale_i2c(&rtd_device, &rtd_scale));

  EZO_ARDUINO_CHECK_OK("send_rtd_read", ezo_rtd_send_read_i2c(&rtd_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_rtd_response",
                       ezo_rtd_read_response_i2c(&rtd_device, rtd_scale.scale, &rtd_reading));

  if (!ezo_arduino_rtd_reading_to_celsius(&rtd_reading, &source_temperature_c)) {
    ezo_arduino_fail_fast("convert_rtd_temperature", EZO_ERR_PROTOCOL);
  }

  EZO_ARDUINO_CHECK_OK("send_ph_temperature_set",
                       ezo_ph_send_temperature_set_i2c(&ph_device, source_temperature_c, 2U, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_ph_temperature_query",
                       ezo_ph_send_temperature_query_i2c(&ph_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_ph_temperature_query",
                       ezo_ph_read_temperature_i2c(&ph_device, &ph_temperature));

  EZO_ARDUINO_CHECK_OK("send_ph_read", ezo_ph_send_read_i2c(&ph_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_ph_response", ezo_ph_read_response_i2c(&ph_device, &ph_reading));

  Serial.print(F("source_scale="));
  Serial.println(ezo_arduino_rtd_scale_name(rtd_reading.scale));
  Serial.print(F("source_temperature="));
  Serial.println(rtd_reading.temperature, 3);
  Serial.print(F("source_temperature_c="));
  Serial.println(source_temperature_c, 3);
  Serial.print(F("applied_temperature_compensation_c="));
  Serial.println(ph_temperature.temperature_c, 3);
  Serial.print(F("ph="));
  Serial.println(ph_reading.ph, 3);
  Serial.println();
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_rtd_device",
                       ezo_arduino_i2c_init_device(&rtd_device, RTD_I2C_ADDRESS, &wire_context));
  EZO_ARDUINO_CHECK_OK("init_ph_device",
                       ezo_arduino_i2c_init_device(&ph_device, PH_I2C_ADDRESS, &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
    return;
  }
  if ((unsigned long)(millis() - last_cycle_started_at_ms) < POLL_INTERVAL_MS) {
    return;
  }

  last_cycle_started_at_ms = millis();
  run_cycle();
}
