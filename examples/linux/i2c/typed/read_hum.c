/*
Purpose: simple typed Linux I2C humidity read with an explicit output-config query.
Defaults: /dev/i2c-1 and the HUM default address 111.
Assumptions: the connected device is a humidity circuit in I2C mode.
Next: read ../commissioning/readiness_check.c for output and calibration state.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_hum.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_hum_output_config_t output_config;
  ezo_hum_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 111U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_hum_send_output_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_hum_read_output_config_i2c(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_hum_send_read_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_hum_read_response_i2c(&session.device, output_config.enabled_mask, &reading);
  }

  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_hum", result);
  }

  printf("transport=i2c\n");
  printf("product=HUM\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("output_mask=%u\n", (unsigned)output_config.enabled_mask);
  if ((reading.present_mask & EZO_HUM_OUTPUT_HUMIDITY) != 0U) {
    printf("relative_humidity_percent=%.3f\n", reading.relative_humidity_percent);
  }
  if ((reading.present_mask & EZO_HUM_OUTPUT_AIR_TEMPERATURE) != 0U) {
    printf("air_temperature_c=%.3f\n", reading.air_temperature_c);
  }
  if ((reading.present_mask & EZO_HUM_OUTPUT_DEW_POINT) != 0U) {
    printf("dew_point_c=%.3f\n", reading.dew_point_c);
  }
  return 0;
}
