/*
Purpose: simple typed Linux UART humidity read with an explicit output-config query.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is a humidity circuit and response-code mode can be bootstrapped.
Next: read ../commissioning/readiness_check.c for output and calibration state.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_hum.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_hum_output_config_t output_config;
  ezo_hum_reading_t reading;
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

  result = ezo_example_uart_bootstrap_response_codes(&session.device, EZO_PRODUCT_HUM, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_hum_send_output_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_hum_read_output_config_uart(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_hum_send_read_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_hum_read_response_uart(&session.device, output_config.enabled_mask, &reading);
  }

  ezo_example_close_uart(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_hum", result);
  }

  printf("transport=uart\n");
  printf("product=HUM\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("output_mask=%u\n", (unsigned)output_config.enabled_mask);
  if ((reading.present_mask & EZO_HUM_OUTPUT_HUMIDITY) != 0U) {
    printf("relative_humidity_percent=%.3f\n", reading.relative_humidity_percent);
  }
  if ((reading.present_mask & EZO_HUM_OUTPUT_AIR_TEMPERATURE) != 0U) {
    printf("air_temperature_c=%.3f\n", reading.air_temperature_c);
  }
  if ((reading.present_mask & EZO_HUM_OUTPUT_DEW_POINT) != 0U) {
    printf("dew_point_c=%.3f\n", reading.dew_point_c);
  }
  return 0;
}
