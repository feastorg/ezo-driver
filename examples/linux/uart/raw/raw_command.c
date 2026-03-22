/*
Purpose: minimal Linux UART raw line-ownership example.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the device is reachable over UART and not held by another process.
Next: read ../commissioning/inspect_device.c for explicit bootstrap and metadata checks.
*/

#include "example_base.h"
#include "example_uart.h"

#include <stdio.h>
#include <string.h>

static int line_equals(const char *buffer, size_t buffer_len, const char *text) {
  size_t expected_len = strlen(text);
  return buffer_len == expected_len && memcmp(buffer, text, expected_len) == 0;
}

int main(int argc, char **argv) {
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  ezo_uart_response_kind_t terminal_kind = EZO_UART_RESPONSE_UNKNOWN;
  char response[64];
  size_t response_len = 0;
  int next_arg = 0;
  int bootstrap_set_applied = 0;
  ezo_result_t result = EZO_OK;

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

  result = ezo_uart_discard_input(&session.device);
  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("discard_input", result);
  }

  result = ezo_uart_send_command(&session.device, "*OK,?", EZO_COMMAND_GENERIC, &hint);
  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("send_response_code_query", result);
  }
  ezo_example_wait_hint(&hint);

  result = ezo_uart_read_line(&session.device, response, sizeof(response), &response_len, &kind);
  if (result != EZO_OK || kind != EZO_UART_RESPONSE_DATA) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("read_response_code_query", result != EZO_OK ? result :
                                                                       EZO_ERR_PROTOCOL);
  }

  if (line_equals(response, response_len, "?*OK,0")) {
    result = ezo_uart_send_command(&session.device, "*OK,1", EZO_COMMAND_GENERIC, &hint);
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("send_response_code_set", result);
    }
    ezo_example_wait_hint(&hint);

    result = ezo_uart_read_terminal_response(&session.device, &terminal_kind);
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("read_response_code_ack", result);
    }
    bootstrap_set_applied = 1;
  } else if (!line_equals(response, response_len, "?*OK,1")) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("parse_response_code_query", EZO_ERR_PROTOCOL);
  }

  result = ezo_uart_send_command(&session.device, "i", EZO_COMMAND_GENERIC, &hint);
  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("send_info_query", result);
  }
  ezo_example_wait_hint(&hint);

  result = ezo_uart_read_line(&session.device, response, sizeof(response), &response_len, &kind);
  if (result != EZO_OK || kind != EZO_UART_RESPONSE_DATA) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("read_info_line", result != EZO_OK ? result :
                                                             EZO_ERR_PROTOCOL);
  }

  result = ezo_uart_read_terminal_response(&session.device, &terminal_kind);
  ezo_example_close_uart(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_terminal_response", result);
  }

  printf("transport=uart\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("response_codes_before_bootstrap=%s\n",
         bootstrap_set_applied ? "disabled" : "enabled");
  printf("bootstrap_set_applied=%d\n", bootstrap_set_applied);
  printf("terminal_response=%s\n", ezo_example_uart_response_kind_name(terminal_kind));
  printf("response=%.*s\n", (int)response_len, response);
  return 0;
}
