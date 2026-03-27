/*
Purpose: drive a full RTD + EC + pressure compensation chain for D.O. on Arduino I2C.
Defaults: Wire on the board default pins, RTD at 102, EC at 100, D.O. at 97, and pressure 101.325 kPa.
Assumptions: EC salinity output is enabled and the measured temperature/salinity are representative of the D.O. probe environment.
Next: read ../do_salinity_comp_from_ec/do_salinity_comp_from_ec.ino for the narrower Arduino salinity-only flow and the Linux D.O. temperature-comp examples for the transport-complete host-side variants.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_do.h>
#include <ezo_ec.h>
#include <ezo_rtd.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const unsigned long POLL_INTERVAL_MS = 2000UL;
static const uint8_t RTD_I2C_ADDRESS = 102U;
static const uint8_t EC_I2C_ADDRESS = 100U;
static const uint8_t DO_I2C_ADDRESS = 97U;
static const float PRESSURE_KPA = 101.325f;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t rtd_device;
static ezo_i2c_device_t ec_device;
static ezo_i2c_device_t do_device;
static unsigned long startup_started_at_ms = 0;
static unsigned long last_cycle_started_at_ms = 0;

static void run_cycle() {
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_ec_output_config_t ec_output_config;
  ezo_ec_reading_t ec_reading;
  ezo_do_output_config_t do_output_config;
  ezo_do_temperature_compensation_t do_temperature;
  ezo_do_salinity_compensation_t do_salinity;
  ezo_do_pressure_compensation_t do_pressure;
  ezo_do_reading_t do_reading;
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

  EZO_ARDUINO_CHECK_OK("send_ec_output_query", ezo_ec_send_output_query_i2c(&ec_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_ec_output_query", ezo_ec_read_output_config_i2c(&ec_device, &ec_output_config));
  EZO_ARDUINO_CHECK_OK("send_ec_read", ezo_ec_send_read_i2c(&ec_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_ec_response",
                       ezo_ec_read_response_i2c(&ec_device, ec_output_config.enabled_mask, &ec_reading));

  if ((ec_reading.present_mask & EZO_EC_OUTPUT_SALINITY) == 0U) {
    ezo_arduino_fail_fast("missing_ec_salinity_output", EZO_ERR_PROTOCOL);
  }

  EZO_ARDUINO_CHECK_OK("send_do_output_query", ezo_do_send_output_query_i2c(&do_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_do_output_query", ezo_do_read_output_config_i2c(&do_device, &do_output_config));

  EZO_ARDUINO_CHECK_OK("send_do_temperature_set",
                       ezo_do_send_temperature_set_i2c(&do_device, source_temperature_c, 2U, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_do_salinity_set",
                       ezo_do_send_salinity_set_i2c(
                           &do_device, ec_reading.salinity_ppt, EZO_DO_SALINITY_UNIT_PPT, 2U, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_do_pressure_set",
                       ezo_do_send_pressure_set_i2c(&do_device, PRESSURE_KPA, 2U, &hint));
  ezo_arduino_wait_hint(&hint);

  EZO_ARDUINO_CHECK_OK("send_do_temperature_query", ezo_do_send_temperature_query_i2c(&do_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_do_temperature_query", ezo_do_read_temperature_i2c(&do_device, &do_temperature));
  EZO_ARDUINO_CHECK_OK("send_do_salinity_query", ezo_do_send_salinity_query_i2c(&do_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_do_salinity_query", ezo_do_read_salinity_i2c(&do_device, &do_salinity));
  EZO_ARDUINO_CHECK_OK("send_do_pressure_query", ezo_do_send_pressure_query_i2c(&do_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_do_pressure_query", ezo_do_read_pressure_i2c(&do_device, &do_pressure));
  EZO_ARDUINO_CHECK_OK("send_do_read", ezo_do_send_read_i2c(&do_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_do_response",
                       ezo_do_read_response_i2c(&do_device, do_output_config.enabled_mask, &do_reading));

  Serial.print(F("source_temperature_c="));
  Serial.println(source_temperature_c, 3);
  ezo_arduino_print_ec_reading(F("source_ec_"), &ec_reading);
  Serial.print(F("applied_temperature_compensation_c="));
  Serial.println(do_temperature.temperature_c, 3);
  Serial.print(F("applied_salinity_value="));
  Serial.println(do_salinity.value, 3);
  Serial.print(F("applied_salinity_unit="));
  Serial.println(ezo_arduino_do_salinity_unit_name(do_salinity.unit));
  Serial.print(F("applied_pressure_kpa="));
  Serial.println(do_pressure.pressure_kpa, 3);
  ezo_arduino_print_do_reading(F("target_"), &do_reading);
  Serial.println(F("vendor_note_ec_measurements_can_interfere=1"));
  Serial.println();
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_rtd_device",
                       ezo_arduino_i2c_init_device(&rtd_device, RTD_I2C_ADDRESS, &wire_context));
  EZO_ARDUINO_CHECK_OK("init_ec_device",
                       ezo_arduino_i2c_init_device(&ec_device, EC_I2C_ADDRESS, &wire_context));
  EZO_ARDUINO_CHECK_OK("init_do_device",
                       ezo_arduino_i2c_init_device(&do_device, DO_I2C_ADDRESS, &wire_context));
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
