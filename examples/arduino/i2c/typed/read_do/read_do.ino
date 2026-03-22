/*
Purpose: simple Arduino I2C dissolved-oxygen read with an explicit output-config query.
Defaults: Wire on the board default pins and address 97.
Assumptions: the device is a D.O. circuit already in I2C mode.
Next: read ../../commissioning/inspect_device/inspect_device.ino for setup checks.
*/

#include <Wire.h>

#include <ezo_do.h>
#include <ezo_i2c_arduino_wire.h>

enum {
  STARTUP_SETTLE_MS = 1000U
};

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static ezo_do_output_config_t output_config;
static unsigned long startup_started_at_ms = 0;
static uint8_t read_requested = 0;

static void fail_fast(ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

  Serial.print("driver_error=");
  Serial.println((int)result);
  while (true) {
  }
}

static void wait_hint(const ezo_timing_hint_t *hint) {
  delay(hint->wait_ms);
}

static void request_output_config() {
  ezo_timing_hint_t hint;

  fail_fast(ezo_do_send_output_query_i2c(&device, &hint));
  wait_hint(&hint);
  fail_fast(ezo_do_read_output_config_i2c(&device, &output_config));
}

static void request_reading() {
  ezo_timing_hint_t hint;

  fail_fast(ezo_do_send_read_i2c(&device, &hint));
  wait_hint(&hint);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  fail_fast(ezo_arduino_wire_context_init(&wire_context, &Wire));
  fail_fast(ezo_device_init(&device, 97, ezo_arduino_wire_transport(), &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_do_reading_t reading;

  if (read_requested == 0U) {
    if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
      return;
    }

    request_output_config();
    request_reading();
    read_requested = 1U;
    return;
  }

  fail_fast(ezo_do_read_response_i2c(&device, output_config.enabled_mask, &reading));

  Serial.print("output_mask=");
  Serial.println((unsigned long)output_config.enabled_mask);
  if ((reading.present_mask & EZO_DO_OUTPUT_MG_L) != 0U) {
    Serial.print("milligrams_per_liter=");
    Serial.println(reading.milligrams_per_liter, 3);
  }
  if ((reading.present_mask & EZO_DO_OUTPUT_PERCENT_SATURATION) != 0U) {
    Serial.print("percent_saturation=");
    Serial.println(reading.percent_saturation, 3);
  }

  delay(1000);
  request_reading();
}
