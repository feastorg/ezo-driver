/*
Purpose: simple typed Linux I2C pH read.
Defaults: /dev/i2c-1 and the pH default address 99.
Assumptions: the connected device is a pH circuit in I2C mode.
Next: read ../commissioning/readiness_check.c or ../advanced/ph_workflow.c.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_ph.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_ph_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 99U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_ph_send_read_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_response_i2c(&session.device, &reading);
  }

  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_ph", result);
  }

  printf("transport=i2c\n");
  printf("product=pH\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("ph=%.3f\n", reading.ph);
  return 0;
}
