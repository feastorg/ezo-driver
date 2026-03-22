/*
Purpose: simple typed Linux UART EC read with an explicit output-config query.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is an EC circuit and response-code mode can be bootstrapped.
Next: read ../commissioning/readiness_check.c or ../advanced/do_workflow.c for config-heavy flows.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_ec.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_ec_output_config_t output_config;
  ezo_ec_reading_t reading;
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

  result = ezo_example_uart_bootstrap_response_codes(&session.device, EZO_PRODUCT_EC, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_ec_send_output_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_output_config_uart(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_read_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_uart(&session.device, output_config.enabled_mask, &reading);
  }

  ezo_example_close_uart(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_ec", result);
  }

  printf("transport=uart\n");
  printf("product=EC\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
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
