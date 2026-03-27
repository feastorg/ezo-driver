/*
Purpose: simple Arduino I2C EC read with an explicit output-config query.
Defaults: Wire on the board default pins and address 100.
Assumptions: the device is an EC circuit already in I2C mode.
Next: read ../../advanced/ec_workflow/ec_workflow.ino for probe-K, TDS-factor, and calibration state.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_ec.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const uint8_t EC_I2C_ADDRESS = 100U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static ezo_ec_output_config_t output_config;
static unsigned long startup_started_at_ms = 0;
static uint8_t read_requested = 0U;

static void request_output_config() {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_output_query", ezo_ec_send_output_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_output_query", ezo_ec_read_output_config_i2c(&device, &output_config));
}

static void request_reading() {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_read", ezo_ec_send_read_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_device",
                       ezo_arduino_i2c_init_device(&device, EC_I2C_ADDRESS, &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_ec_reading_t reading;

  if (read_requested == 0U) {
    if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
      return;
    }

    request_output_config();
    request_reading();
    read_requested = 1U;
    return;
  }

  EZO_ARDUINO_CHECK_OK("read_response",
                       ezo_ec_read_response_i2c(&device, output_config.enabled_mask, &reading));
  Serial.print(F("output_mask="));
  Serial.println((unsigned long)output_config.enabled_mask);
  ezo_arduino_print_ec_reading(F(""), &reading);

  delay(1000);
  request_reading();
}
