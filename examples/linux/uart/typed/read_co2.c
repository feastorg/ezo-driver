/*
Purpose: simple typed Linux UART carbon dioxide read with an explicit output-config query.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is an carbon dioxide sensor and response-code mode can be bootstrapped.
Next: read docs/ezo/products/co2.md for what this module covers.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_co2.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_co2_output_config_t output_config;
  ezo_co2_reading_t reading;
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

  result = ezo_example_uart_bootstrap_response_codes(&session.device, EZO_PRODUCT_CO2, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_co2_send_output_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_co2_read_output_config_uart(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_co2_send_read_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_co2_read_response_uart(&session.device, output_config.enabled_mask, &reading);
  }

  ezo_example_close_uart(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_co2", result);
  }

  printf("transport=uart\n");
  printf("product=CO2\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("output_mask=%u\n", (unsigned)output_config.enabled_mask);
  printf("warming=%u\n", (unsigned)reading.warming);
  if ((reading.present_mask & EZO_CO2_OUTPUT_PPM) != 0U) {
    printf("ppm=%.0f\n", reading.ppm);
  }
  if ((reading.present_mask & EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE) != 0U) {
    printf("internal_temperature_c=%.2f\n", reading.internal_temperature_c);
  }
  return 0;
}
