#include "example_control.h"

#include "example_base.h"

#include <stdio.h>

static void ezo_example_print_shared_control_result(const char *step, ezo_result_t result) {
  printf("%s_result=%s\n", step, ezo_result_name(result));
  printf("%s_result_code=%d\n", step, (int)result);
}

static void ezo_example_print_text_debug(const char *label, const uint8_t *buffer, size_t len) {
  size_t i = 0;

  printf("%s=", label);
  for (i = 0; i < len; ++i) {
    const unsigned value = (unsigned)buffer[i];

    if (value == '\\') {
      printf("\\\\");
    } else if (value == '\r') {
      printf("\\r");
    } else if (value == '\n') {
      printf("\\n");
    } else if (value == '\t') {
      printf("\\t");
    } else if (value >= 32U && value <= 126U) {
      printf("%c", (int)value);
    } else {
      printf("\\x%02X", value);
    }
  }
  printf("\n");
}

static void ezo_example_print_hex_debug(const char *label, const uint8_t *buffer, size_t len) {
  size_t i = 0;

  printf("%s=", label);
  for (i = 0; i < len; ++i) {
    if (i > 0) {
      printf(" ");
    }
    printf("%02X", (unsigned)buffer[i]);
  }
  printf("\n");
}

static ezo_result_t ezo_example_query_name_i2c(ezo_i2c_device_t *device,
                                               ezo_product_id_t product_id,
                                               ezo_control_name_t *name_out) {
  ezo_timing_hint_t hint;
  char text_buffer[EZO_I2C_MAX_TEXT_RESPONSE_CAPACITY];
  size_t text_len = 0;
  ezo_device_status_t device_status = EZO_STATUS_UNKNOWN;
  ezo_result_t send_result = EZO_OK;
  ezo_result_t read_result = EZO_OK;
  ezo_result_t parse_result = EZO_OK;

  if (device == NULL || name_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  send_result = ezo_control_send_name_query_i2c(device, product_id, &hint);
  if (send_result != EZO_OK) {
    ezo_example_print_shared_control_result("shared_control_name_query_send", send_result);
    return send_result;
  }

  ezo_example_wait_hint(&hint);
  read_result =
      ezo_read_response(device, text_buffer, sizeof(text_buffer), &text_len, &device_status);
  if (read_result != EZO_OK) {
    ezo_example_print_shared_control_result("shared_control_name_query_read", read_result);
    printf("shared_control_name_query_device_status=%s\n",
           ezo_device_status_name(device_status));
    printf("shared_control_name_query_device_status_code=%u\n", (unsigned)device_status);
    return read_result;
  }

  parse_result = ezo_control_parse_name(text_buffer, text_len, name_out);
  if (parse_result != EZO_OK) {
    printf("shared_control_name_query_device_status=%s\n",
           ezo_device_status_name(device_status));
    printf("shared_control_name_query_device_status_code=%u\n", (unsigned)device_status);
    printf("shared_control_name_query_text_len=%u\n", (unsigned)text_len);
    ezo_example_print_text_debug("shared_control_name_query_text", (const uint8_t *)text_buffer,
                                 text_len);
    ezo_example_print_hex_debug("shared_control_name_query_text_hex",
                                (const uint8_t *)text_buffer,
                                text_len);
    ezo_example_print_shared_control_result("shared_control_name_query_parse", parse_result);
    return parse_result;
  }

  return EZO_OK;
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
