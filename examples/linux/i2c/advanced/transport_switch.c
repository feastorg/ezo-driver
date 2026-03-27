/*
Purpose: inspect I2C transport state and stage a software switch from I2C mode to UART mode.
Defaults: /dev/i2c-1, address 99, and target UART baud 9600.
Assumptions: the selected device is in I2C mode and protocol lock is disabled before apply.
Next: read ../commissioning/inspect_device.c or the UART transport_switch.c example after reconnecting on the new transport.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_control.h"

#include <stdio.h>

static ezo_result_t query_info(ezo_i2c_device_t *device, ezo_device_info_t *info_out) {
  ezo_timing_hint_t hint;
  ezo_result_t result = ezo_control_send_info_query_i2c(device, EZO_PRODUCT_UNKNOWN, &hint);

  if (result != EZO_OK) {
    return result;
  }

  ezo_example_wait_hint(&hint);
  return ezo_control_read_info_i2c(device, info_out);
}

int main(int argc, char **argv) {
  uint32_t target_baud = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_device_info_t info;
  ezo_timing_hint_t hint;
  ezo_control_protocol_lock_status_t protocol_lock;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_i2c_options(argc, argv, 99U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address] [--baud=9600] [--apply]\n", argv[0]);
    return 1;
  }

  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");
  value = ezo_example_find_option_value(argc, argv, next_arg, "--baud=");
  if (value != NULL && !ezo_example_parse_uint32_arg(value, &target_baud)) {
    fprintf(stderr, "invalid --baud value\n");
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = query_info(&session.device, &info);
  if (result == EZO_OK) {
    result = ezo_control_send_protocol_lock_query_i2c(&session.device, info.product_id, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_protocol_lock_i2c(&session.device, &protocol_lock);
  }
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_switch_state", result);
  }

  printf("transport=i2c\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("product_code=%s\n", info.product_code);
  printf("firmware_version=%s\n", info.firmware_version);
  printf("current_protocol_lock_enabled=%u\n", (unsigned)protocol_lock.enabled);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_target_transport=uart\n");
  printf("planned_target_baud=%u\n", (unsigned)target_baud);

  if (apply_requested) {
    if (protocol_lock.enabled != 0U) {
      ezo_example_close_i2c(&session);
      fprintf(stderr, "protocol lock must be disabled before switching transport\n");
      return 1;
    }

    result = ezo_control_send_switch_to_uart_i2c(&session.device,
                                                 info.product_id,
                                                 target_baud,
                                                 &hint);
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("switch_to_uart", result);
    }
    ezo_example_wait_hint(&hint);

    printf("post_transport=uart\n");
    printf("reconnect_uart_baud=%u\n", (unsigned)target_baud);
    printf("post_query_skipped_reason=transport_changed\n");
  }

  ezo_example_close_i2c(&session);
  return 0;
}
