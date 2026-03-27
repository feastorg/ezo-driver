/*
Purpose: inspect shared I2C control state and optionally apply name, LED, protocol-lock, find, sleep, or factory-reset operations.
Defaults: /dev/i2c-1 and address 99.
Assumptions: the selected device is already in I2C mode and responds at the given address.
Next: read transport_switch.c for the software path that changes transport mode.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_control.h"

#include <stdio.h>

static ezo_result_t query_info(ezo_i2c_device_t *device, ezo_device_info_t *info_out) {
  ezo_timing_hint_t hint;
  ezo_result_t result = EZO_OK;

  result = ezo_control_send_info_query_i2c(device, EZO_PRODUCT_UNKNOWN, &hint);
  if (result != EZO_OK) {
    return result;
  }
  ezo_example_wait_hint(&hint);
  return ezo_control_read_info_i2c(device, info_out);
}

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_device_info_t info;
  ezo_timing_hint_t hint;
  ezo_control_name_t name;
  ezo_control_status_t status;
  ezo_control_led_status_t led;
  ezo_control_protocol_lock_status_t protocol_lock;
  const char *planned_name = NULL;
  int planned_led = -1;
  int planned_protocol_lock = -1;
  int send_find = 0;
  int send_sleep = 0;
  int send_factory_reset = 0;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_i2c_options(argc, argv, 99U, &options, &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [address] [--set-name=tank] [--set-led=0|1] "
            "[--set-protocol-lock=0|1] [--send-find] [--send-sleep] [--factory-reset] "
            "[--apply]\n",
            argv[0]);
    return 1;
  }

  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");
  send_find = ezo_example_has_flag(argc, argv, next_arg, "--send-find");
  send_sleep = ezo_example_has_flag(argc, argv, next_arg, "--send-sleep");
  send_factory_reset = ezo_example_has_flag(argc, argv, next_arg, "--factory-reset");
  planned_name = ezo_example_find_option_value(argc, argv, next_arg, "--set-name=");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-led=");
  if (value != NULL) {
    uint32_t parsed = 0;
    if (!ezo_example_parse_uint32_arg(value, &parsed) || parsed > 1U) {
      fprintf(stderr, "invalid --set-led value\n");
      return 1;
    }
    planned_led = (int)parsed;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-protocol-lock=");
  if (value != NULL) {
    uint32_t parsed = 0;
    if (!ezo_example_parse_uint32_arg(value, &parsed) || parsed > 1U) {
      fprintf(stderr, "invalid --set-protocol-lock value\n");
      return 1;
    }
    planned_protocol_lock = (int)parsed;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = query_info(&session.device, &info);
  if (result == EZO_OK) {
    result = ezo_control_send_name_query_i2c(&session.device, info.product_id, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_name_i2c(&session.device, &name);
  }
  if (result == EZO_OK) {
    result = ezo_control_send_status_query_i2c(&session.device, info.product_id, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_status_i2c(&session.device, &status);
  }
  if (result == EZO_OK) {
    result = ezo_control_send_led_query_i2c(&session.device, info.product_id, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_led_i2c(&session.device, &led);
  }
  if (result == EZO_OK) {
    result = ezo_control_send_protocol_lock_query_i2c(&session.device, info.product_id, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_protocol_lock_i2c(&session.device, &protocol_lock);
  }
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_current_state", result);
  }

  printf("transport=i2c\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("product_code=%s\n", info.product_code);
  printf("firmware_version=%s\n", info.firmware_version);
  printf("current_name=%s\n", name.name);
  printf("current_restart_code=%c\n", status.restart_code);
  printf("current_supply_voltage_v=%.3f\n", status.supply_voltage);
  printf("current_led_enabled=%u\n", (unsigned)led.enabled);
  printf("current_protocol_lock_enabled=%u\n", (unsigned)protocol_lock.enabled);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_name=%s\n", planned_name != NULL ? planned_name : "(unchanged)");
  printf("planned_led=%d\n", planned_led);
  printf("planned_protocol_lock=%d\n", planned_protocol_lock);
  printf("planned_send_find=%d\n", send_find);
  printf("planned_send_sleep=%d\n", send_sleep);
  printf("planned_factory_reset=%d\n", send_factory_reset);

  if (apply_requested) {
    if (planned_name != NULL) {
      result = ezo_control_send_name_set_i2c(&session.device, info.product_id, planned_name, &hint);
    }
    if (result == EZO_OK && planned_name != NULL) {
      ezo_example_wait_hint(&hint);
    }
    if (result == EZO_OK && planned_led >= 0) {
      result = ezo_control_send_led_set_i2c(&session.device,
                                            info.product_id,
                                            (uint8_t)planned_led,
                                            &hint);
    }
    if (result == EZO_OK && planned_led >= 0) {
      ezo_example_wait_hint(&hint);
    }
    if (result == EZO_OK && planned_protocol_lock >= 0) {
      result = ezo_control_send_protocol_lock_set_i2c(&session.device,
                                                      info.product_id,
                                                      (uint8_t)planned_protocol_lock,
                                                      &hint);
    }
    if (result == EZO_OK && planned_protocol_lock >= 0) {
      ezo_example_wait_hint(&hint);
    }
    if (result == EZO_OK && send_find) {
      result = ezo_control_send_find_i2c(&session.device, info.product_id, &hint);
    }
    if (result == EZO_OK && send_find) {
      ezo_example_wait_hint(&hint);
    }
    if (result == EZO_OK && send_sleep) {
      result = ezo_control_send_sleep_i2c(&session.device, info.product_id, &hint);
    }
    if (result == EZO_OK && send_sleep) {
      ezo_example_wait_hint(&hint);
    }
    if (result == EZO_OK && send_factory_reset) {
      result = ezo_control_send_factory_reset_i2c(&session.device, info.product_id, &hint);
    }
    if (result == EZO_OK && send_factory_reset) {
      ezo_example_wait_hint(&hint);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("apply_control_changes", result);
    }

    if (send_sleep || send_factory_reset) {
      printf("post_query_skipped_reason=%s\n", send_sleep ? "sleep_requested" : "factory_reset_requested");
    } else {
      result = ezo_control_send_name_query_i2c(&session.device, info.product_id, &hint);
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_control_read_name_i2c(&session.device, &name);
      }
      if (result == EZO_OK) {
        result = ezo_control_send_led_query_i2c(&session.device, info.product_id, &hint);
      }
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_control_read_led_i2c(&session.device, &led);
      }
      if (result == EZO_OK) {
        result = ezo_control_send_protocol_lock_query_i2c(&session.device, info.product_id, &hint);
      }
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_control_read_protocol_lock_i2c(&session.device, &protocol_lock);
      }
      if (result != EZO_OK) {
        ezo_example_close_i2c(&session);
        return ezo_example_print_error("post_query", result);
      }

      printf("post_name=%s\n", name.name);
      printf("post_led_enabled=%u\n", (unsigned)led.enabled);
      printf("post_protocol_lock_enabled=%u\n", (unsigned)protocol_lock.enabled);
    }
  }

  ezo_example_close_i2c(&session);
  return 0;
}
