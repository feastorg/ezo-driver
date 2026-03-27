/*
Purpose: inspect an RTD-driven pH temperature-compensation chain and optionally apply the source temperature.
Defaults: RTD on /dev/ttyUSB0, pH on /dev/ttyUSB1, both at 9600 baud.
Assumptions: both devices are reachable over UART and response-code mode can be bootstrapped.
Next: read ph_workflow.c and ec_temp_comp_from_rtd.c for adjacent single-device and cross-device compensation flows.
*/

#include "example_base.h"
#include "example_products.h"
#include "example_uart.h"

#include "ezo_ph.h"
#include "ezo_rtd.h"

#include <stdio.h>

int main(int argc, char **argv) {
  const char *rtd_path = EZO_EXAMPLE_UART_DEFAULT_PATH;
  const char *ph_path = "/dev/ttyUSB1";
  ezo_uart_posix_baud_t rtd_baud = EZO_UART_POSIX_BAUD_9600;
  ezo_uart_posix_baud_t ph_baud = EZO_UART_POSIX_BAUD_9600;
  uint32_t rtd_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  uint32_t ph_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  ezo_example_uart_session_t rtd_session;
  ezo_example_uart_session_t ph_session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_ph_temperature_compensation_t ph_temperature;
  ezo_ph_reading_t ph_reading;
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
    const char *value = ezo_example_find_option_value(argc, argv, index, "--ph-path=");
    if (value != NULL) {
      ph_path = value;
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
    const char *value = ezo_example_find_option_value(argc, argv, index, "--ph-baud=");
    if (value != NULL && !ezo_example_parse_baud_arg(value, &ph_baud, &ph_baud_rate)) {
      fprintf(stderr, "invalid --ph-baud value\n");
      return 1;
    }
  }

  result = ezo_example_open_uart(rtd_path, rtd_baud, &rtd_session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_rtd_uart", result);
  }

  result = ezo_example_open_uart(ph_path, ph_baud, &ph_session);
  if (result != EZO_OK) {
    ezo_example_close_uart(&rtd_session);
    return ezo_example_print_error("open_ph_uart", result);
  }

  result = ezo_example_uart_bootstrap_response_codes(&rtd_session.device, EZO_PRODUCT_RTD, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_example_uart_bootstrap_response_codes(&ph_session.device, EZO_PRODUCT_PH, NULL, NULL);
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
    result = ezo_ph_send_temperature_query_uart(&ph_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_temperature_uart(&ph_session.device, &ph_temperature);
  }
  if (result == EZO_OK) {
    result = ezo_ph_send_read_uart(&ph_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_response_uart(&ph_session.device, &ph_reading);
  }

  if (result != EZO_OK) {
    ezo_example_close_uart(&ph_session);
    ezo_example_close_uart(&rtd_session);
    return ezo_example_print_error("inspect_chain_state", result);
  }

  printf("transport=uart\n");
  printf("rtd_path=%s\n", rtd_path);
  printf("rtd_baud_rate=%u\n", (unsigned)rtd_baud_rate);
  printf("ph_path=%s\n", ph_path);
  printf("ph_baud_rate=%u\n", (unsigned)ph_baud_rate);
  printf("source_scale=%s\n", ezo_example_rtd_scale_name(rtd_reading.scale));
  printf("source_temperature=%.3f\n", rtd_reading.temperature);
  printf("source_temperature_c=%.3f\n", source_temperature_c);
  printf("current_target_temperature_compensation_c=%.3f\n", ph_temperature.temperature_c);
  printf("current_target_reading_ph=%.3f\n", ph_reading.ph);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", source_temperature_c);

  if (apply_requested) {
    result = ezo_ph_send_temperature_set_uart(&ph_session.device, source_temperature_c, 3, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&ph_session.device);
    }
    if (result == EZO_OK) {
      result = ezo_ph_send_temperature_query_uart(&ph_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_read_temperature_uart(&ph_session.device, &ph_temperature);
    }
    if (result == EZO_OK) {
      result = ezo_ph_send_read_uart(&ph_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_read_response_uart(&ph_session.device, &ph_reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&ph_session);
      ezo_example_close_uart(&rtd_session);
      return ezo_example_print_error("apply_temperature_compensation", result);
    }

    printf("post_target_temperature_compensation_c=%.3f\n", ph_temperature.temperature_c);
    printf("post_target_reading_ph=%.3f\n", ph_reading.ph);
  }

  ezo_example_close_uart(&ph_session);
  ezo_example_close_uart(&rtd_session);
  return 0;
}
