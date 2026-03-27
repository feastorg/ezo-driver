/*
Purpose: inspect an RTD-driven D.O. temperature-compensation chain and optionally apply the source temperature.
Defaults: RTD on /dev/ttyUSB0, D.O. on /dev/ttyUSB1, both at 9600 baud.
Assumptions: both devices are reachable over UART and response-code mode can be bootstrapped.
Next: read do_workflow.c and do_full_compensation_chain.c for adjacent single-device and full-chain compensation flows.
*/

#include "example_base.h"
#include "example_products.h"
#include "example_uart.h"

#include "ezo_do.h"
#include "ezo_rtd.h"

#include <stdio.h>

int main(int argc, char **argv) {
  const char *rtd_path = EZO_EXAMPLE_UART_DEFAULT_PATH;
  const char *do_path = "/dev/ttyUSB1";
  ezo_uart_posix_baud_t rtd_baud = EZO_UART_POSIX_BAUD_9600;
  ezo_uart_posix_baud_t do_baud = EZO_UART_POSIX_BAUD_9600;
  uint32_t rtd_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  uint32_t do_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  ezo_example_uart_session_t rtd_session;
  ezo_example_uart_session_t do_session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_do_output_config_t do_output_config;
  ezo_do_temperature_compensation_t do_temperature;
  ezo_do_reading_t do_reading;
  double source_temperature_c = 0.0;
  ezo_result_t result = EZO_OK;
  int index = 1;
  int apply_requested = 0;

  apply_requested = ezo_example_has_flag(argc, argv, index, "--apply");

  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--rtd-path=");
    if (value != NULL) {
      rtd_path = value;
    }
  }
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--do-path=");
    if (value != NULL) {
      do_path = value;
    }
  }
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--rtd-baud=");
    if (value != NULL && !ezo_example_parse_baud_arg(value, &rtd_baud, &rtd_baud_rate)) {
      fprintf(stderr, "invalid --rtd-baud value\n");
      return 1;
    }
  }
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--do-baud=");
    if (value != NULL && !ezo_example_parse_baud_arg(value, &do_baud, &do_baud_rate)) {
      fprintf(stderr, "invalid --do-baud value\n");
      return 1;
    }
  }

  result = ezo_example_open_uart(rtd_path, rtd_baud, &rtd_session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_rtd_uart", result);
  }

  result = ezo_example_open_uart(do_path, do_baud, &do_session);
  if (result != EZO_OK) {
    ezo_example_close_uart(&rtd_session);
    return ezo_example_print_error("open_do_uart", result);
  }

  result = ezo_example_uart_bootstrap_response_codes(&rtd_session.device, EZO_PRODUCT_RTD, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_example_uart_bootstrap_response_codes(&do_session.device, EZO_PRODUCT_DO, NULL, NULL);
  }
  if (result == EZO_OK) {
    result = ezo_rtd_send_scale_query_uart(&rtd_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_scale_uart(&rtd_session.device, &rtd_scale);
  }
  if (result == EZO_OK) {
    result = ezo_rtd_send_read_uart(&rtd_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_response_uart(&rtd_session.device, rtd_scale.scale, &rtd_reading);
  }
  if (result == EZO_OK &&
      !ezo_example_rtd_reading_to_celsius(&rtd_reading, &source_temperature_c)) {
    result = EZO_ERR_PROTOCOL;
  }
  if (result == EZO_OK) {
    result = ezo_do_send_output_query_uart(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_output_config_uart(&do_session.device, &do_output_config);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_temperature_query_uart(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_temperature_uart(&do_session.device, &do_temperature);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_read_uart(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_response_uart(&do_session.device, do_output_config.enabled_mask, &do_reading);
  }

  if (result != EZO_OK) {
    ezo_example_close_uart(&do_session);
    ezo_example_close_uart(&rtd_session);
    return ezo_example_print_error("inspect_chain_state", result);
  }

  printf("transport=uart\n");
  printf("rtd_path=%s\n", rtd_path);
  printf("rtd_baud_rate=%u\n", (unsigned)rtd_baud_rate);
  printf("do_path=%s\n", do_path);
  printf("do_baud_rate=%u\n", (unsigned)do_baud_rate);
  printf("source_scale=%s\n", ezo_example_rtd_scale_name(rtd_reading.scale));
  printf("source_temperature=%.3f\n", rtd_reading.temperature);
  printf("source_temperature_c=%.3f\n", source_temperature_c);
  printf("target_output_mask=%u\n", (unsigned)do_output_config.enabled_mask);
  printf("current_target_temperature_compensation_c=%.3f\n", do_temperature.temperature_c);
  ezo_example_print_do_reading("current_target_", &do_reading);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", source_temperature_c);

  if (apply_requested) {
    result = ezo_do_send_temperature_set_uart(&do_session.device, source_temperature_c, 3, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&do_session.device);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_temperature_query_uart(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_temperature_uart(&do_session.device, &do_temperature);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_read_uart(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_response_uart(&do_session.device, do_output_config.enabled_mask, &do_reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&do_session);
      ezo_example_close_uart(&rtd_session);
      return ezo_example_print_error("apply_temperature_compensation", result);
    }

    printf("post_target_temperature_compensation_c=%.3f\n", do_temperature.temperature_c);
    ezo_example_print_do_reading("post_target_", &do_reading);
  }

  ezo_example_close_uart(&do_session);
  ezo_example_close_uart(&rtd_session);
  return 0;
}
