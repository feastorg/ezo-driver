/*
Purpose: simple typed Linux I2C dissolved-oxygen read with an explicit output-config query.
Defaults: /dev/i2c-1 and the D.O. default address 97.
Assumptions: the connected device is a D.O. circuit in I2C mode.
Next: read ../advanced/do_workflow.c for compensation and output-setting flows.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_do.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_do_output_config_t output_config;
  ezo_do_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 97U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_do_send_output_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_output_config_i2c(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_read_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_response_i2c(&session.device, output_config.enabled_mask, &reading);
  }

  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_do", result);
  }

  printf("transport=i2c\n");
  printf("product=D.O.\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("output_mask=%u\n", (unsigned)output_config.enabled_mask);
  if ((reading.present_mask & EZO_DO_OUTPUT_MG_L) != 0U) {
    printf("milligrams_per_liter=%.3f\n", reading.milligrams_per_liter);
  }
  if ((reading.present_mask & EZO_DO_OUTPUT_PERCENT_SATURATION) != 0U) {
    printf("percent_saturation=%.3f\n", reading.percent_saturation);
  }
  return 0;
}
