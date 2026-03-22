/*
Purpose: inspect a UART device and print repo metadata plus shared control state.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the device is in UART mode or still at its shipping UART default.
Next: read readiness_check.c for product-specific setup state.
*/

#include "example_base.h"
#include "example_product_uart.h"
#include "example_uart.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_device_info_t info;
  const ezo_product_metadata_t *metadata = NULL;
  uint8_t response_codes_before = 0;
  uint8_t response_codes_after = 0;
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

  result = ezo_example_uart_bootstrap_response_codes(&session.device,
                                                     EZO_PRODUCT_UNKNOWN,
                                                     &response_codes_before,
                                                     &response_codes_after);
  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("bootstrap_response_codes", result);
  }

  result = ezo_example_query_info_uart(&session.device, &info);
  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("query_info", result);
  }

  metadata = ezo_product_get_metadata(info.product_id);

  printf("transport=uart\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("response_codes_before_bootstrap=%u\n", (unsigned)response_codes_before);
  printf("response_codes_after_bootstrap=%u\n", (unsigned)response_codes_after);
  ezo_example_print_device_info(&info);
  ezo_example_print_product_metadata(metadata);

  result = ezo_example_print_shared_control_uart(&session.device, info.product_id);
  ezo_example_close_uart(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("shared_control", result);
  }

  return 0;
}
