/*
Purpose: simple typed Linux I2C EC read with an explicit output-config query.
Defaults: /dev/i2c-1 and the EC default address 100.
Assumptions: the connected device is an EC circuit in I2C mode.
Next: read ../commissioning/readiness_check.c or ../advanced/do_workflow.c for config-heavy flows.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_ec.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_ec_output_config_t output_config;
  ezo_ec_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 100U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_ec_send_output_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_output_config_i2c(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_read_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_i2c(&session.device, output_config.enabled_mask, &reading);
  }

  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_ec", result);
  }

  printf("transport=i2c\n");
  printf("product=EC\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("output_mask=%u\n", (unsigned)output_config.enabled_mask);
  if ((reading.present_mask & EZO_EC_OUTPUT_CONDUCTIVITY) != 0U) {
    printf("conductivity_us_cm=%.3f\n", reading.conductivity_us_cm);
  }
  if ((reading.present_mask & EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS) != 0U) {
    printf("total_dissolved_solids_ppm=%.3f\n", reading.total_dissolved_solids_ppm);
  }
  if ((reading.present_mask & EZO_EC_OUTPUT_SALINITY) != 0U) {
    printf("salinity_ppt=%.3f\n", reading.salinity_ppt);
  }
  if ((reading.present_mask & EZO_EC_OUTPUT_SPECIFIC_GRAVITY) != 0U) {
    printf("specific_gravity=%.3f\n", reading.specific_gravity);
  }
  return 0;
}
