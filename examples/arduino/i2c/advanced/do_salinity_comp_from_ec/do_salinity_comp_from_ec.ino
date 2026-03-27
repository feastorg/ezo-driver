/*
Purpose: drive D.O. salinity compensation from a live EC reading on Arduino I2C.
Defaults: Wire on the board default pins, EC at 100, and D.O. at 97.
Assumptions: EC salinity output is enabled and both circuits share the same I2C bus.
Next: read ../do_full_compensation_chain/do_full_compensation_chain.ino for the fuller RTD + EC + pressure chain.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_do.h>
#include <ezo_ec.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const unsigned long POLL_INTERVAL_MS = 2000UL;
static const uint8_t EC_I2C_ADDRESS = 100U;
static const uint8_t DO_I2C_ADDRESS = 97U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t ec_device;
static ezo_i2c_device_t do_device;
static unsigned long startup_started_at_ms = 0;
static unsigned long last_cycle_started_at_ms = 0;

static void run_cycle() {
  ezo_timing_hint_t hint;
  ezo_ec_output_config_t ec_output_config;
  ezo_ec_reading_t ec_reading;
  ezo_do_output_config_t do_output_config;
  ezo_do_salinity_compensation_t do_salinity;
  ezo_do_reading_t do_reading;

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

  EZO_ARDUINO_CHECK_OK("send_do_salinity_set",
                       ezo_do_send_salinity_set_i2c(
                           &do_device, ec_reading.salinity_ppt, EZO_DO_SALINITY_UNIT_PPT, 2U, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("send_do_salinity_query", ezo_do_send_salinity_query_i2c(&do_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_do_salinity_query", ezo_do_read_salinity_i2c(&do_device, &do_salinity));

  EZO_ARDUINO_CHECK_OK("send_do_read", ezo_do_send_read_i2c(&do_device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_do_response",
                       ezo_do_read_response_i2c(&do_device, do_output_config.enabled_mask, &do_reading));

  ezo_arduino_print_ec_reading(F("source_ec_"), &ec_reading);
  Serial.print(F("applied_salinity_value="));
  Serial.println(do_salinity.value, 3);
  Serial.print(F("applied_salinity_unit="));
  Serial.println(ezo_arduino_do_salinity_unit_name(do_salinity.unit));
  ezo_arduino_print_do_reading(F("target_"), &do_reading);
  Serial.println(F("vendor_note_ec_measurements_can_interfere=1"));
  Serial.println();
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
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
