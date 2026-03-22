/*
Purpose: minimal Linux I2C raw command/response example.
Defaults: /dev/i2c-1 and address 99.
Assumptions: the device is already in I2C mode at the selected address.
Next: read ../commissioning/inspect_device.c for identity and metadata checks.
*/

#include "example_base.h"
#include "example_i2c.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  char response[64];
  size_t response_len = 0;
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

  result = ezo_send_command(&session.device, "name,?", EZO_COMMAND_GENERIC, &hint);
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("send_command", result);
  }

  ezo_example_wait_hint(&hint);
  result = ezo_read_response(&session.device,
                             response,
                             sizeof(response),
                             &response_len,
                             &status);
  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("read_response", result);
  }

  printf("transport=i2c\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("command=name,?\n");
  printf("device_status_name=%s\n", ezo_device_status_name(status));
  printf("device_status_code=%u\n", (unsigned)status);
  printf("response=%.*s\n", (int)response_len, response);
  return 0;
}
