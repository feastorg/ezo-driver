/*
Purpose: simple typed Linux I2C ORP read.
Defaults: /dev/i2c-1 and the ORP default address 98.
Assumptions: the connected device is an ORP circuit in I2C mode.
Next: read ../commissioning/readiness_check.c for calibration and range state.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_orp.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_orp_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 98U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_orp_send_read_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_orp_read_response_i2c(&session.device, &reading);
  }

  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_orp", result);
  }

  printf("transport=i2c\n");
  printf("product=ORP\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("millivolts=%.3f\n", reading.millivolts);
  return 0;
}
