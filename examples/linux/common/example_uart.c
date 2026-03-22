#include "example_base.h"
#include "example_uart.h"

ezo_result_t ezo_example_open_uart(const char *device_path,
                                   ezo_uart_posix_baud_t baud,
                                   ezo_example_uart_session_t *session_out) {
  ezo_result_t result = EZO_OK;

  if (device_path == NULL || session_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_uart_posix_serial_open(&session_out->serial, device_path, baud, 500U);
  if (result != EZO_OK) {
    return result;
  }

  result = ezo_uart_device_init(&session_out->device,
                                ezo_uart_posix_serial_transport(),
                                &session_out->serial);
  if (result != EZO_OK) {
    ezo_uart_posix_serial_close(&session_out->serial);
    return result;
  }

  return EZO_OK;
}

void ezo_example_close_uart(ezo_example_uart_session_t *session) {
  if (session == NULL) {
    return;
  }

  ezo_uart_posix_serial_close(&session->serial);
}

ezo_result_t ezo_example_uart_bootstrap_response_codes(
    ezo_uart_device_t *device,
    ezo_product_id_t product_id,
    uint8_t *enabled_before_out,
    uint8_t *enabled_after_out) {
  ezo_timing_hint_t hint;
  ezo_control_response_code_status_t response_code;
  ezo_result_t result = EZO_OK;

  if (enabled_before_out != NULL) {
    *enabled_before_out = 0;
  }
  if (enabled_after_out != NULL) {
    *enabled_after_out = 0;
  }

  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_uart_discard_input(device);
  if (result != EZO_OK) {
    return result;
  }

  result = ezo_control_send_response_code_query_uart(device, product_id, &hint);
  if (result != EZO_OK) {
    return result;
  }
  ezo_example_wait_hint(&hint);

  result = ezo_control_read_response_code_uart(device, &response_code);
  if (result != EZO_OK) {
    return result;
  }

  if (enabled_before_out != NULL) {
    *enabled_before_out = response_code.enabled;
  }
  if (enabled_after_out != NULL) {
    *enabled_after_out = response_code.enabled;
  }

  if (response_code.enabled != 0U) {
    return EZO_OK;
  }

  result = ezo_control_send_response_code_set_uart(device, product_id, 1, &hint);
  if (result != EZO_OK) {
    return result;
  }
  ezo_example_wait_hint(&hint);

  result = ezo_uart_read_ok(device);
  if (result != EZO_OK) {
    return result;
  }

  if (enabled_after_out != NULL) {
    *enabled_after_out = 1;
  }
  return EZO_OK;
}
