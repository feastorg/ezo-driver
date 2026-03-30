#include "example_control.h"

#include "example_base.h"

#include <stdio.h>

static void ezo_example_print_shared_control_result(const char *step, ezo_result_t result) {
  printf("%s_result=%s\n", step, ezo_result_name(result));
  printf("%s_result_code=%d\n", step, (int)result);
}

static ezo_result_t ezo_example_query_name_i2c(ezo_i2c_device_t *device,
                                               ezo_product_id_t product_id,
                                               ezo_control_name_t *name_out) {
  ezo_timing_hint_t hint;
  ezo_result_t result = EZO_OK;

  if (device == NULL || name_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_control_send_name_query_i2c(device, product_id, &hint);
  if (result != EZO_OK) {
    return result;
  }

  ezo_example_wait_hint(&hint);
  return ezo_control_read_name_i2c(device, name_out);
}

ezo_result_t ezo_example_query_info_i2c(ezo_i2c_device_t *device, ezo_device_info_t *info_out) {
  ezo_timing_hint_t hint;
  ezo_result_t result = EZO_OK;

  if (device == NULL || info_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_control_send_info_query_i2c(device, EZO_PRODUCT_UNKNOWN, &hint);
  if (result != EZO_OK) {
    return result;
  }

  ezo_example_wait_hint(&hint);
  return ezo_control_read_info_i2c(device, info_out);
}

ezo_result_t ezo_example_print_shared_control_i2c(ezo_i2c_device_t *device,
                                                  ezo_product_id_t product_id) {
  ezo_timing_hint_t hint;
  ezo_control_name_t name;
  ezo_control_status_t status;
  ezo_control_led_status_t led;
  ezo_control_protocol_lock_status_t protocol_lock;
  ezo_result_t result = EZO_OK;
  int shared_control_complete = 1;

  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_example_query_name_i2c(device, product_id, &name);
  if (result == EZO_OK) {
    printf("device_name=%s\n", name.name);
  } else {
    shared_control_complete = 0;
    ezo_example_print_shared_control_result("shared_control_name_query", result);
  }

  result = ezo_control_send_status_query_i2c(device, product_id, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_status_i2c(device, &status);
  }
  if (result == EZO_OK) {
    printf("restart_code=%c\n", status.restart_code);
    printf("supply_voltage_v=%.3f\n", status.supply_voltage);
  } else {
    shared_control_complete = 0;
    ezo_example_print_shared_control_result("shared_control_status_query", result);
  }

  result = ezo_control_send_led_query_i2c(device, product_id, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_led_i2c(device, &led);
  }
  if (result == EZO_OK) {
    printf("led_enabled=%u\n", (unsigned)led.enabled);
  } else {
    shared_control_complete = 0;
    ezo_example_print_shared_control_result("shared_control_led_query", result);
  }

  result = ezo_control_send_protocol_lock_query_i2c(device, product_id, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_control_read_protocol_lock_i2c(device, &protocol_lock);
  }
  if (result == EZO_OK) {
    printf("protocol_lock_enabled=%u\n", (unsigned)protocol_lock.enabled);
  } else {
    shared_control_complete = 0;
    ezo_example_print_shared_control_result("shared_control_protocol_lock_query", result);
  }

  printf("shared_control_complete=%u\n", (unsigned)(shared_control_complete != 0));
  return EZO_OK;
}
