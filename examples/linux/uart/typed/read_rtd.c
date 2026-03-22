/*
Purpose: simple typed Linux UART RTD read with an explicit scale query.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is an RTD circuit and response-code mode can be bootstrapped.
Next: read ../advanced/rtd_workflow.c for scale, logger, and memory flows.
*/

#include "example_base.h"
#include "example_uart.h"

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
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t scale;
  ezo_rtd_reading_t reading;
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

  result = ezo_example_uart_bootstrap_response_codes(&session.device, EZO_PRODUCT_RTD, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_rtd_send_scale_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_scale_uart(&session.device, &scale);
  }
  if (result == EZO_OK) {
    result = ezo_rtd_send_read_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_response_uart(&session.device, scale.scale, &reading);
  }

  ezo_example_close_uart(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_rtd", result);
  }

  printf("transport=uart\n");
  printf("product=RTD\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("scale=%s\n", scale_name(scale.scale));
  printf("temperature=%.3f\n", reading.temperature);
  return 0;
}
