/*
Purpose: simple typed Linux I2C RTD read with an explicit scale query.
Defaults: /dev/i2c-1 and the RTD default address 102.
Assumptions: the connected device is an RTD circuit in I2C mode.
Next: read ../advanced/rtd_workflow.c for scale, logger, and memory flows.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_rtd.h"

#include <stdio.h>

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

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t scale;
  ezo_rtd_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 102U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_rtd_send_scale_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_scale_i2c(&session.device, &scale);
  }
  if (result == EZO_OK) {
    result = ezo_rtd_send_read_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_response_i2c(&session.device, scale.scale, &reading);
  }

  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_rtd", result);
  }

  printf("transport=i2c\n");
  printf("product=RTD\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("scale=%s\n", scale_name(scale.scale));
  printf("temperature=%.3f\n", reading.temperature);
  return 0;
}
