/*
Purpose: simple typed Linux I2C oxygen read with an explicit output-config query.
Defaults: /dev/i2c-1 and the O2 default address 108.
Assumptions: the connected device is an oxygen sensor in I2C mode.
Next: read docs/ezo/products/o2.md for what this module covers.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_o2.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_o2_output_config_t output_config;
  ezo_o2_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 108U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_o2_send_output_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_o2_read_output_config_i2c(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_o2_send_read_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_o2_read_response_i2c(&session.device, output_config.enabled_mask, &reading);
  }

  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_o2", result);
  }

  printf("transport=i2c\n");
  printf("product=O2\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("output_mask=%u\n", (unsigned)output_config.enabled_mask);
  if ((reading.present_mask & EZO_O2_OUTPUT_PERCENT) != 0U) {
    printf("percent=%.2f\n", reading.percent);
  }
  if ((reading.present_mask & EZO_O2_OUTPUT_PPT) != 0U) {
    printf("parts_per_thousand=%.1f\n", reading.parts_per_thousand);
  }
  return 0;
}
