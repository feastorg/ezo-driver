/*
Purpose: simple typed Linux UART ORP read.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is an ORP circuit and response-code mode can be bootstrapped.
Next: read ../commissioning/readiness_check.c for calibration and range state.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_orp.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_orp_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_uart_options(argc,
                                      argv,
                                      EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE,
                                      &options,
                                      &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [baud]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_uart(options.device_path, options.baud, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_uart", result);
  }

  result =
      ezo_example_uart_bootstrap_response_codes(&session.device, EZO_PRODUCT_ORP, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_orp_send_read_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_orp_read_response_uart(&session.device, &reading);
  }

  ezo_example_close_uart(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_orp", result);
  }

  printf("transport=uart\n");
  printf("product=ORP\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("millivolts=%.3f\n", reading.millivolts);
  return 0;
}
