/*
Purpose: drive EC temperature compensation from a live RTD reading on Arduino I2C.
Defaults: Wire on the board default pins, RTD at address 102, and EC at address 100.
Assumptions: both circuits share the same I2C bus and this sketch is allowed to update the EC temperature-compensation state.
Next: read ../../../linux/i2c/typed/read_ec.c for the smallest host-side EC typed read path and ../../../linux/i2c/advanced/ec_temp_comp_from_rtd.c for the matching Linux advanced flow.
*/

#include <Wire.h>

#include <ezo_ec.h>
#include <ezo_i2c_arduino_wire.h>
#include <ezo_rtd.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const unsigned long POLL_INTERVAL_MS = 2000UL;
static const uint8_t RTD_I2C_ADDRESS = 102U;
static const uint8_t EC_I2C_ADDRESS = 100U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t rtd_device;
static ezo_i2c_device_t ec_device;
static unsigned long startup_started_at_ms = 0;
static unsigned long last_cycle_started_at_ms = 0;

static void fail_fast(const char *step, ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

  Serial.print("driver_error_step=");
  Serial.println(step);
  Serial.print("driver_error_name=");
  Serial.println(ezo_result_name(result));
  Serial.print("driver_error_code=");
  Serial.println((int)result);
  while (true) {
  }
}

#define CHECK_OK(step, expr) fail_fast(step, (expr))

static void wait_hint(const ezo_timing_hint_t *hint) {
  delay(hint->wait_ms);
}

static const char *scale_name(ezo_rtd_scale_t scale) {
  switch (scale) {
    case EZO_RTD_SCALE_CELSIUS:
      return "celsius";
    case EZO_RTD_SCALE_KELVIN:
      return "kelvin";
    case EZO_RTD_SCALE_FAHRENHEIT:
      return "fahrenheit";
    default:
      return "unknown";
  }
}

static int reading_to_celsius(const ezo_rtd_reading_t *reading, float *temperature_c_out) {
  if (reading == NULL || temperature_c_out == NULL) {
    return 0;
  }

  switch (reading->scale) {
    case EZO_RTD_SCALE_CELSIUS:
      *temperature_c_out = (float)reading->temperature;
      return 1;
    case EZO_RTD_SCALE_KELVIN:
      *temperature_c_out = (float)(reading->temperature - 273.15);
      return 1;
    case EZO_RTD_SCALE_FAHRENHEIT:
      *temperature_c_out = (float)((reading->temperature - 32.0) * (5.0 / 9.0));
      return 1;
    case EZO_RTD_SCALE_UNKNOWN:
    default:
      return 0;
  }
}

static void print_ec_reading(const ezo_ec_reading_t *reading) {
  if ((reading->present_mask & EZO_EC_OUTPUT_CONDUCTIVITY) != 0U) {
    Serial.print("conductivity_us_cm=");
    Serial.println(reading->conductivity_us_cm, 3);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS) != 0U) {
    Serial.print("total_dissolved_solids_ppm=");
    Serial.println(reading->total_dissolved_solids_ppm, 3);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SALINITY) != 0U) {
    Serial.print("salinity_ppt=");
    Serial.println(reading->salinity_ppt, 3);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SPECIFIC_GRAVITY) != 0U) {
    Serial.print("specific_gravity=");
    Serial.println(reading->specific_gravity, 3);
  }
}

static void run_cycle() {
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_ec_output_config_t ec_output_config;
  ezo_ec_temperature_compensation_t ec_temperature;
  ezo_ec_reading_t ec_reading;
  float source_temperature_c = 0.0f;

  CHECK_OK("send_rtd_scale_query", ezo_rtd_send_scale_query_i2c(&rtd_device, &hint));
  wait_hint(&hint);
  CHECK_OK("read_rtd_scale_query", ezo_rtd_read_scale_i2c(&rtd_device, &rtd_scale));

  CHECK_OK("send_rtd_read", ezo_rtd_send_read_i2c(&rtd_device, &hint));
  wait_hint(&hint);
  CHECK_OK("read_rtd_response",
           ezo_rtd_read_response_i2c(&rtd_device, rtd_scale.scale, &rtd_reading));

  if (!reading_to_celsius(&rtd_reading, &source_temperature_c)) {
    fail_fast("convert_rtd_temperature", EZO_ERR_PROTOCOL);
  }

  CHECK_OK("send_ec_output_query", ezo_ec_send_output_query_i2c(&ec_device, &hint));
  wait_hint(&hint);
  CHECK_OK("read_ec_output_query", ezo_ec_read_output_config_i2c(&ec_device, &ec_output_config));

  CHECK_OK("send_ec_temperature_set",
           ezo_ec_send_temperature_set_i2c(&ec_device, source_temperature_c, 3, &hint));
  wait_hint(&hint);

  CHECK_OK("send_ec_temperature_query", ezo_ec_send_temperature_query_i2c(&ec_device, &hint));
  wait_hint(&hint);
  CHECK_OK("read_ec_temperature_query", ezo_ec_read_temperature_i2c(&ec_device, &ec_temperature));

  CHECK_OK("send_ec_read", ezo_ec_send_read_i2c(&ec_device, &hint));
  wait_hint(&hint);
  CHECK_OK("read_ec_response",
           ezo_ec_read_response_i2c(&ec_device, ec_output_config.enabled_mask, &ec_reading));

  Serial.print("source_scale=");
  Serial.println(scale_name(rtd_reading.scale));
  Serial.print("source_temperature=");
  Serial.println(rtd_reading.temperature, 3);
  Serial.print("source_temperature_c=");
  Serial.println(source_temperature_c, 3);
  Serial.print("target_output_mask=");
  Serial.println((unsigned long)ec_output_config.enabled_mask);
  Serial.print("applied_temperature_compensation_c=");
  Serial.println(ec_temperature.temperature_c, 3);
  print_ec_reading(&ec_reading);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  CHECK_OK("init_wire_context", ezo_arduino_wire_context_init(&wire_context, &Wire));
  CHECK_OK("init_rtd_device",
           ezo_device_init(&rtd_device,
                           RTD_I2C_ADDRESS,
                           ezo_arduino_wire_transport(),
                           &wire_context));
  CHECK_OK("init_ec_device",
           ezo_device_init(&ec_device,
                           EC_I2C_ADDRESS,
                           ezo_arduino_wire_transport(),
                           &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
    return;
  }

  if ((unsigned long)(millis() - last_cycle_started_at_ms) < POLL_INTERVAL_MS) {
    return;
  }

  last_cycle_started_at_ms = millis();
  run_cycle();
}
