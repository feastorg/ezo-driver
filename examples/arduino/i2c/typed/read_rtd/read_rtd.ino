/*
Purpose: simple Arduino I2C RTD read with an explicit scale query.
Defaults: Wire on the board default pins and address 102.
Assumptions: the device is an RTD circuit already in I2C mode.
Next: read ../../advanced/rtd_workflow/rtd_workflow.ino for scale, logger, and memory state.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_rtd.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const uint8_t RTD_I2C_ADDRESS = 102U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static ezo_rtd_scale_status_t scale_status;
static unsigned long startup_started_at_ms = 0;
static uint8_t read_requested = 0U;

static void request_scale() {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_scale_query", ezo_rtd_send_scale_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_scale_query", ezo_rtd_read_scale_i2c(&device, &scale_status));
}

static void request_reading() {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_read", ezo_rtd_send_read_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_device",
                       ezo_arduino_i2c_init_device(&device, RTD_I2C_ADDRESS, &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_rtd_reading_t reading;

  if (read_requested == 0U) {
    if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
      return;
    }

    request_scale();
    request_reading();
    read_requested = 1U;
    return;
  }

  EZO_ARDUINO_CHECK_OK("read_response",
                       ezo_rtd_read_response_i2c(&device, scale_status.scale, &reading));
  Serial.print(F("scale="));
  Serial.println(ezo_arduino_rtd_scale_name(reading.scale));
  Serial.print(F("temperature="));
  Serial.println(reading.temperature, 3);

  delay(1000);
  request_reading();
}
