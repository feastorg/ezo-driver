/*
Purpose: inspect a full RTD + EC + pressure driven D.O. compensation chain and optionally apply all three inputs.
Defaults: RTD on /dev/ttyUSB0, EC on /dev/ttyUSB1, D.O. on /dev/ttyUSB2, all at 9600 baud, and pressure 101.325 kPa.
Assumptions: all devices are reachable over UART, response-code mode can be bootstrapped, and EC salinity output is enabled before apply.
Next: read do_temp_comp_from_rtd.c and do_salinity_comp_from_ec.c for the narrower compensation-chain examples.
*/

#include "example_base.h"
#include "example_products.h"
#include "example_uart.h"

#include "ezo_do.h"
#include "ezo_ec.h"
#include "ezo_rtd.h"

#include <stdio.h>

int main(int argc, char **argv) {
  const char *rtd_path = EZO_EXAMPLE_UART_DEFAULT_PATH;
  const char *ec_path = "/dev/ttyUSB1";
  const char *do_path = "/dev/ttyUSB2";
  ezo_uart_posix_baud_t rtd_baud = EZO_UART_POSIX_BAUD_9600;
  ezo_uart_posix_baud_t ec_baud = EZO_UART_POSIX_BAUD_9600;
  ezo_uart_posix_baud_t do_baud = EZO_UART_POSIX_BAUD_9600;
  uint32_t rtd_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  uint32_t ec_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  uint32_t do_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  double pressure_kpa = 101.325;
  ezo_example_uart_session_t rtd_session;
  ezo_example_uart_session_t ec_session;
  ezo_example_uart_session_t do_session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_ec_output_config_t ec_output_config;
  ezo_ec_reading_t ec_reading;
  ezo_do_output_config_t do_output_config;
  ezo_do_temperature_compensation_t do_temperature;
  ezo_do_salinity_compensation_t do_salinity;
  ezo_do_pressure_compensation_t do_pressure;
  ezo_do_reading_t do_reading;
  double source_temperature_c = 0.0;
  double source_salinity_ppt = 0.0;
  int source_salinity_available = 0;
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
    const char *value = ezo_example_find_option_value(argc, argv, index, "--ec-path=");
    if (value != NULL) {
      ec_path = value;
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
    const char *value = ezo_example_find_option_value(argc, argv, index, "--ec-baud=");
    if (value != NULL && !ezo_example_parse_baud_arg(value, &ec_baud, &ec_baud_rate)) {
      fprintf(stderr, "invalid --ec-baud value\n");
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
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--pressure-kpa=");
    if (value != NULL && !ezo_example_parse_double_arg(value, &pressure_kpa)) {
      fprintf(stderr, "invalid --pressure-kpa value\n");
      return 1;
    }
  }

  result = ezo_example_open_uart(rtd_path, rtd_baud, &rtd_session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_rtd_uart", result);
  }
  result = ezo_example_open_uart(ec_path, ec_baud, &ec_session);
  if (result != EZO_OK) {
    ezo_example_close_uart(&rtd_session);
    return ezo_example_print_error("open_ec_uart", result);
  }
  result = ezo_example_open_uart(do_path, do_baud, &do_session);
  if (result != EZO_OK) {
    ezo_example_close_uart(&ec_session);
    ezo_example_close_uart(&rtd_session);
    return ezo_example_print_error("open_do_uart", result);
  }

  result = ezo_example_uart_bootstrap_response_codes(&rtd_session.device, EZO_PRODUCT_RTD, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_example_uart_bootstrap_response_codes(&ec_session.device, EZO_PRODUCT_EC, NULL, NULL);
  }
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
    result = ezo_ec_send_output_query_uart(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_output_config_uart(&ec_session.device, &ec_output_config);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_read_uart(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_uart(&ec_session.device, ec_output_config.enabled_mask, &ec_reading);
  }
  if (result == EZO_OK) {
    source_salinity_available = (ec_reading.present_mask & EZO_EC_OUTPUT_SALINITY) != 0U;
    if (source_salinity_available) {
      source_salinity_ppt = ec_reading.salinity_ppt;
    }
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
    result = ezo_do_send_salinity_query_uart(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_salinity_uart(&do_session.device, &do_salinity);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_pressure_query_uart(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_pressure_uart(&do_session.device, &do_pressure);
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
    ezo_example_close_uart(&ec_session);
    ezo_example_close_uart(&rtd_session);
    return ezo_example_print_error("inspect_chain_state", result);
  }

  printf("transport=uart\n");
  printf("rtd_path=%s\n", rtd_path);
  printf("rtd_baud_rate=%u\n", (unsigned)rtd_baud_rate);
  printf("ec_path=%s\n", ec_path);
  printf("ec_baud_rate=%u\n", (unsigned)ec_baud_rate);
  printf("do_path=%s\n", do_path);
  printf("do_baud_rate=%u\n", (unsigned)do_baud_rate);
  printf("source_temperature_scale=%s\n", ezo_example_rtd_scale_name(rtd_reading.scale));
  printf("source_temperature=%.3f\n", rtd_reading.temperature);
  printf("source_temperature_c=%.3f\n", source_temperature_c);
  printf("source_ec_output_mask=%u\n", (unsigned)ec_output_config.enabled_mask);
  ezo_example_print_ec_reading("source_ec_", &ec_reading);
  printf("source_salinity_available=%d\n", source_salinity_available);
  if (source_salinity_available) {
    printf("source_salinity_ppt=%.3f\n", source_salinity_ppt);
  }
  printf("current_target_temperature_compensation_c=%.3f\n", do_temperature.temperature_c);
  printf("current_target_salinity_value=%.3f\n", do_salinity.value);
  printf("current_target_salinity_unit=%s\n", ezo_example_do_salinity_unit_name(do_salinity.unit));
  printf("current_target_pressure_kpa=%.3f\n", do_pressure.pressure_kpa);
  ezo_example_print_do_reading("current_target_", &do_reading);
  printf("vendor_note_ec_measurements_can_interfere=1\n");
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_pressure_kpa=%.3f\n", pressure_kpa);
  if (!source_salinity_available) {
    printf("apply_blocked_reason=ec_salinity_output_disabled\n");
  } else {
    printf("planned_salinity_ppt=%.3f\n", source_salinity_ppt);
  }
  printf("planned_temperature_c=%.3f\n", source_temperature_c);

  if (apply_requested) {
    if (!source_salinity_available) {
      fprintf(stderr, "EC salinity output must be enabled before --apply\n");
      ezo_example_close_uart(&do_session);
      ezo_example_close_uart(&ec_session);
      ezo_example_close_uart(&rtd_session);
      return 1;
    }

    result = ezo_do_send_temperature_set_uart(&do_session.device, source_temperature_c, 3, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&do_session.device);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_salinity_set_uart(&do_session.device,
                                             source_salinity_ppt,
                                             EZO_DO_SALINITY_UNIT_PPT,
                                             3,
                                             &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&do_session.device);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_pressure_set_uart(&do_session.device, pressure_kpa, 3, &hint);
    }
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
      result = ezo_do_send_salinity_query_uart(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_salinity_uart(&do_session.device, &do_salinity);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_pressure_query_uart(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_pressure_uart(&do_session.device, &do_pressure);
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
      ezo_example_close_uart(&ec_session);
      ezo_example_close_uart(&rtd_session);
      return ezo_example_print_error("apply_full_compensation", result);
    }

    printf("post_target_temperature_compensation_c=%.3f\n", do_temperature.temperature_c);
    printf("post_target_salinity_value=%.3f\n", do_salinity.value);
    printf("post_target_salinity_unit=%s\n", ezo_example_do_salinity_unit_name(do_salinity.unit));
    printf("post_target_pressure_kpa=%.3f\n", do_pressure.pressure_kpa);
    ezo_example_print_do_reading("post_target_", &do_reading);
  }

  ezo_example_close_uart(&do_session);
  ezo_example_close_uart(&ec_session);
  ezo_example_close_uart(&rtd_session);
  return 0;
}
