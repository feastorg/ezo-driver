/*
Purpose: inspect UART transport state and stage a software switch to I2C mode or a different UART baud rate.
Defaults: /dev/ttyUSB0 at 9600 baud, target i2c, and the product's default I2C address when known.
Assumptions: the selected device is reachable in UART mode and protocol lock is disabled before apply.
Next: read ../commissioning/inspect_device.c after reconnecting on the new transport or baud rate.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_control.h"
#include "ezo_product.h"

#include <stdio.h>
#include <string.h>

typedef enum {
  TARGET_I2C = 0,
  TARGET_UART
} switch_target_t;

static int parse_target(const char *text, switch_target_t *target_out) {
  if (text == NULL || target_out == NULL) {
    return 0;
  }
  if (strcmp(text, "i2c") == 0) {
    *target_out = TARGET_I2C;
    return 1;
  }
  if (strcmp(text, "uart") == 0) {
    *target_out = TARGET_UART;
    return 1;
  }
  return 0;
}

static ezo_result_t query_info(ezo_uart_device_t *device, ezo_device_info_t *info_out) {
  ezo_timing_hint_t hint;
  ezo_result_t result = ezo_control_send_info_query_uart(device, EZO_PRODUCT_UNKNOWN, &hint);

  if (result != EZO_OK) {
    return result;
  }

  ezo_example_wait_hint(&hint);
  return ezo_control_read_info_uart(device, info_out);
}

int main(int argc, char **argv) {
  switch_target_t target = TARGET_I2C;
  uint8_t target_address = 99U;
  uint32_t target_baud = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_device_info_t info;
  const ezo_product_metadata_t *metadata = NULL;
  ezo_timing_hint_t hint;
  ezo_control_protocol_lock_status_t protocol_lock;
  ezo_control_baud_status_t current_baud;
  ezo_control_response_code_status_t response_code;
  uint8_t response_codes_before = 0;
  uint8_t response_codes_after = 0;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_uart_options(argc,
                                      argv,
                                      EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE,
                                      &options,
                                      &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [baud] [--target=i2c|uart] [--address=99] "
            "[--baud=38400] [--apply]\n",
            argv[0]);
    return 1;
  }

  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--target=");
  if (value != NULL && !parse_target(value, &target)) {
    fprintf(stderr, "invalid --target value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--address=");
  if (value != NULL && !ezo_example_parse_uint8_arg(value, &target_address)) {
    fprintf(stderr, "invalid --address value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--baud=");
  if (value != NULL && !ezo_example_parse_uint32_arg(value, &target_baud)) {
    fprintf(stderr, "invalid --baud value\n");
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
  if (result == EZO_OK) {
    result = query_info(&session.device, &info);
  }
  if (result == EZO_OK) {
    metadata = ezo_product_get_metadata(info.product_id);
    if (value == NULL && metadata != NULL && metadata->default_i2c_address != 0U) {
      target_address = metadata->default_i2c_address;
    }
  }
  if (result == EZO_OK) {
    result = ezo_control_send_protocol_lock_query_uart(&session.device, info.product_id, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_protocol_lock_uart(&session.device, &protocol_lock);
  }
  if (result == EZO_OK) {
    result = ezo_control_send_baud_query_uart(&session.device, info.product_id, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_baud_uart(&session.device, &current_baud);
  }
  if (result == EZO_OK) {
    result = ezo_control_send_response_code_query_uart(&session.device, info.product_id, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_response_code_uart(&session.device, &response_code);
  }
  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("query_switch_state", result);
  }

  printf("transport=uart\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("response_codes_before_bootstrap=%u\n", (unsigned)response_codes_before);
  printf("response_codes_after_bootstrap=%u\n", (unsigned)response_codes_after);
  printf("product_code=%s\n", info.product_code);
  printf("firmware_version=%s\n", info.firmware_version);
  printf("current_protocol_lock_enabled=%u\n", (unsigned)protocol_lock.enabled);
  printf("current_baud_rate=%u\n", (unsigned)current_baud.baud_rate);
  printf("current_response_codes_enabled=%u\n", (unsigned)response_code.enabled);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_target_transport=%s\n", target == TARGET_I2C ? "i2c" : "uart");
  printf("planned_target_address=%u\n", (unsigned)target_address);
  printf("planned_target_baud=%u\n", (unsigned)target_baud);

  if (apply_requested) {
    if (protocol_lock.enabled != 0U) {
      ezo_example_close_uart(&session);
      fprintf(stderr, "protocol lock must be disabled before switching transport\n");
      return 1;
    }

    if (target == TARGET_I2C) {
      result = ezo_control_send_switch_to_i2c_uart(&session.device,
                                                   info.product_id,
                                                   target_address,
                                                   &hint);
    } else {
      result = ezo_control_send_switch_to_uart_uart(&session.device,
                                                    info.product_id,
                                                    target_baud,
                                                    &hint);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("send_transport_switch", result);
    }
    ezo_example_wait_hint(&hint);

    printf("post_transport=%s\n", target == TARGET_I2C ? "i2c" : "uart");
    if (target == TARGET_I2C) {
      printf("reconnect_i2c_address=%u\n", (unsigned)target_address);
    } else {
      printf("reconnect_uart_baud=%u\n", (unsigned)target_baud);
    }
    printf("post_query_skipped_reason=transport_or_link_parameters_changed\n");
  }

  ezo_example_close_uart(&session);
  return 0;
}
