/*
Purpose: inspect an RTD-driven EC temperature-compensation chain and optionally apply the source temperature.
Defaults: RTD on /dev/ttyUSB0, EC on /dev/ttyUSB1, both at 9600 baud.
Assumptions: both devices are reachable over UART and response-code mode can be bootstrapped.
Next: read rtd_workflow.c and do_salinity_comp_from_ec.c for the adjacent cross-device stateful flows.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_ec.h"
#include "ezo_rtd.h"

#include <stdio.h>

static const char *scale_name(ezo_rtd_scale_t scale) {
  switch (scale) {
    case EZO_RTD_SCALE_CELSIUS:
      return "celsius";
    case EZO_RTD_SCALE_KELVIN:
      return "kelvin";
    case EZO_RTD_SCALE_FAHRENHEIT:
      return "fahrenheit";
    default:
      return "unknown";
  }
}

static int reading_to_celsius(const ezo_rtd_reading_t *reading, double *temperature_c_out) {
  if (reading == NULL || temperature_c_out == NULL) {
    return 0;
  }

  switch (reading->scale) {
    case EZO_RTD_SCALE_CELSIUS:
      *temperature_c_out = reading->temperature;
      return 1;
    case EZO_RTD_SCALE_KELVIN:
      *temperature_c_out = reading->temperature - 273.15;
      return 1;
    case EZO_RTD_SCALE_FAHRENHEIT:
      *temperature_c_out = (reading->temperature - 32.0) * (5.0 / 9.0);
      return 1;
    case EZO_RTD_SCALE_UNKNOWN:
    default:
      return 0;
  }
}

static void print_ec_reading(const char *prefix, const ezo_ec_reading_t *reading) {
  if ((reading->present_mask & EZO_EC_OUTPUT_CONDUCTIVITY) != 0U) {
    printf("%sconductivity_us_cm=%.3f\n", prefix, reading->conductivity_us_cm);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS) != 0U) {
    printf("%stotal_dissolved_solids_ppm=%.3f\n", prefix, reading->total_dissolved_solids_ppm);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SALINITY) != 0U) {
    printf("%ssalinity_ppt=%.3f\n", prefix, reading->salinity_ppt);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SPECIFIC_GRAVITY) != 0U) {
    printf("%sspecific_gravity=%.3f\n", prefix, reading->specific_gravity);
  }
}

int main(int argc, char **argv) {
  const char *rtd_path = EZO_EXAMPLE_UART_DEFAULT_PATH;
  const char *ec_path = "/dev/ttyUSB1";
  ezo_uart_posix_baud_t rtd_baud = EZO_UART_POSIX_BAUD_9600;
  ezo_uart_posix_baud_t ec_baud = EZO_UART_POSIX_BAUD_9600;
  uint32_t rtd_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  uint32_t ec_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  ezo_example_uart_session_t rtd_session;
  ezo_example_uart_session_t ec_session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_ec_output_config_t ec_output_config;
  ezo_ec_temperature_compensation_t ec_temperature;
  ezo_ec_reading_t ec_reading;
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
    const char *value = ezo_example_find_option_value(argc, argv, index, "--ec-path=");
    if (value != NULL) {
      ec_path = value;
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

  result = ezo_example_open_uart(rtd_path, rtd_baud, &rtd_session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_rtd_uart", result);
  }

  result = ezo_example_open_uart(ec_path, ec_baud, &ec_session);
  if (result != EZO_OK) {
    ezo_example_close_uart(&rtd_session);
    return ezo_example_print_error("open_ec_uart", result);
  }

  result =
      ezo_example_uart_bootstrap_response_codes(&rtd_session.device, EZO_PRODUCT_RTD, NULL, NULL);
  if (result == EZO_OK) {
    result =
        ezo_example_uart_bootstrap_response_codes(&ec_session.device, EZO_PRODUCT_EC, NULL, NULL);
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
  if (result == EZO_OK && !reading_to_celsius(&rtd_reading, &source_temperature_c)) {
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
    result = ezo_ec_send_temperature_query_uart(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_temperature_uart(&ec_session.device, &ec_temperature);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_read_uart(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_uart(&ec_session.device, ec_output_config.enabled_mask, &ec_reading);
  }

  if (result != EZO_OK) {
    ezo_example_close_uart(&ec_session);
    ezo_example_close_uart(&rtd_session);
    return ezo_example_print_error("inspect_chain_state", result);
  }

  printf("transport=uart\n");
  printf("rtd_path=%s\n", rtd_path);
  printf("rtd_baud_rate=%u\n", (unsigned)rtd_baud_rate);
  printf("ec_path=%s\n", ec_path);
  printf("ec_baud_rate=%u\n", (unsigned)ec_baud_rate);
  printf("source_scale=%s\n", scale_name(rtd_reading.scale));
  printf("source_temperature=%.3f\n", rtd_reading.temperature);
  printf("source_temperature_c=%.3f\n", source_temperature_c);
  printf("target_output_mask=%u\n", (unsigned)ec_output_config.enabled_mask);
  printf("current_target_temperature_compensation_c=%.3f\n", ec_temperature.temperature_c);
  print_ec_reading("current_target_", &ec_reading);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", source_temperature_c);

  if (apply_requested) {
    result = ezo_ec_send_temperature_set_uart(&ec_session.device, source_temperature_c, 3, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&ec_session.device);
    }
    if (result == EZO_OK) {
      result = ezo_ec_send_temperature_query_uart(&ec_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_read_temperature_uart(&ec_session.device, &ec_temperature);
    }
    if (result == EZO_OK) {
      result = ezo_ec_send_read_uart(&ec_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_read_response_uart(&ec_session.device, ec_output_config.enabled_mask, &ec_reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&ec_session);
      ezo_example_close_uart(&rtd_session);
      return ezo_example_print_error("apply_temperature_compensation", result);
    }

    printf("post_target_temperature_compensation_c=%.3f\n", ec_temperature.temperature_c);
    print_ec_reading("post_target_", &ec_reading);
  }

  ezo_example_close_uart(&ec_session);
  ezo_example_close_uart(&rtd_session);
  return 0;
}
