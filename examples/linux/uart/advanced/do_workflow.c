/*
Purpose: inspect D.O. output, compensation, calibration, and one-shot RT,n state, with optional setters.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is a D.O. circuit and response-code mode can be bootstrapped.
Next: read do_calibration.c for staged vendor-aligned low and high calibration steps.
*/

#include "example_base.h"
#include "example_products.h"
#include "example_uart.h"

#include "ezo_do.h"

#include <stdio.h>
#include <string.h>

static int parse_salinity_unit(const char *text, ezo_do_salinity_unit_t *unit_out) {
  if (text == NULL || unit_out == NULL) {
    return 0;
  }
  if (strcmp(text, "ppt") == 0) {
    *unit_out = EZO_DO_SALINITY_UNIT_PPT;
    return 1;
  }
  if (strcmp(text, "us_cm") == 0) {
    *unit_out = EZO_DO_SALINITY_UNIT_MICROSIEMENS;
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  double planned_temperature_c = 25.0;
  double planned_salinity_value = 35.0;
  double planned_pressure_kpa = 101.325;
  double one_shot_temperature_c = 30.0;
  ezo_do_salinity_unit_t planned_salinity_unit = EZO_DO_SALINITY_UNIT_PPT;
  uint8_t enable_percent_saturation_output = 1U;
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_do_output_config_t output_config;
  ezo_do_temperature_compensation_t temperature;
  ezo_do_salinity_compensation_t salinity;
  ezo_do_pressure_compensation_t pressure;
  ezo_do_calibration_status_t calibration;
  ezo_do_reading_t reading;
  ezo_do_reading_t one_shot_reading;
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
            "usage: %s [device_path] [baud] [--set-temperature-c=25.0] "
            "[--set-salinity-value=35.0] [--set-salinity-unit=ppt|us_cm] "
            "[--set-pressure-kpa=101.325] [--set-percent-sat-output=0|1] "
            "[--rt-temperature-c=30.0] [--apply]\n",
            argv[0]);
    return 1;
  }
  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-temperature-c=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &planned_temperature_c)) {
    fprintf(stderr, "invalid --set-temperature-c value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-salinity-value=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &planned_salinity_value)) {
    fprintf(stderr, "invalid --set-salinity-value value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-salinity-unit=");
  if (value != NULL && !parse_salinity_unit(value, &planned_salinity_unit)) {
    fprintf(stderr, "invalid --set-salinity-unit value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-pressure-kpa=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &planned_pressure_kpa)) {
    fprintf(stderr, "invalid --set-pressure-kpa value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-percent-sat-output=");
  if (value != NULL) {
    uint32_t enabled = 0;

    if (!ezo_example_parse_uint32_arg(value, &enabled) || enabled > 1U) {
      fprintf(stderr, "invalid --set-percent-sat-output value\n");
      return 1;
    }

    enable_percent_saturation_output = (uint8_t)enabled;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--rt-temperature-c=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &one_shot_temperature_c)) {
    fprintf(stderr, "invalid --rt-temperature-c value\n");
    return 1;
  }

  result = ezo_example_open_uart(options.device_path, options.baud, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_uart", result);
  }

  result = ezo_example_uart_bootstrap_response_codes(&session.device, EZO_PRODUCT_DO, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_do_send_output_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_output_config_uart(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_temperature_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_temperature_uart(&session.device, &temperature);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_salinity_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_salinity_uart(&session.device, &salinity);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_pressure_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_pressure_uart(&session.device, &pressure);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_calibration_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_calibration_status_uart(&session.device, &calibration);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_read_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_response_uart(&session.device, output_config.enabled_mask, &reading);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_read_with_temp_comp_uart(&session.device,
                                                  one_shot_temperature_c,
                                                  2,
                                                  &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_response_uart(&session.device,
                                       output_config.enabled_mask,
                                       &one_shot_reading);
  }

  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("query_current_state", result);
  }

  printf("transport=uart\n");
  printf("product=D.O.\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("current_output_mask=%u\n", (unsigned)output_config.enabled_mask);
  printf("current_temperature_compensation_c=%.3f\n", temperature.temperature_c);
  printf("current_salinity_value=%.3f\n", salinity.value);
  printf("current_salinity_unit=%s\n", ezo_example_do_salinity_unit_name(salinity.unit));
  printf("current_pressure_kpa=%.3f\n", pressure.pressure_kpa);
  printf("current_calibration_level=%u\n", (unsigned)calibration.level);
  ezo_example_print_do_reading("current_", &reading);
  printf("one_shot_rt_temperature_c=%.3f\n", one_shot_temperature_c);
  ezo_example_print_do_reading("one_shot_rt_", &one_shot_reading);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", planned_temperature_c);
  printf("planned_salinity_value=%.3f\n", planned_salinity_value);
  printf("planned_salinity_unit=%s\n", ezo_example_do_salinity_unit_name(planned_salinity_unit));
  printf("planned_pressure_kpa=%.3f\n", planned_pressure_kpa);
  printf("planned_percent_saturation_output=%s\n",
         ezo_example_bool_name(enable_percent_saturation_output != 0U));

  if (apply_requested) {
    result = ezo_do_send_temperature_set_uart(&session.device, planned_temperature_c, 2, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_salinity_set_uart(&session.device,
                                             planned_salinity_value,
                                             planned_salinity_unit,
                                             2,
                                             &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_pressure_set_uart(&session.device, planned_pressure_kpa, 3, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_output_set_uart(&session.device,
                                           EZO_DO_OUTPUT_PERCENT_SATURATION,
                                           enable_percent_saturation_output,
                                           &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_output_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_output_config_uart(&session.device, &output_config);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_temperature_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_temperature_uart(&session.device, &temperature);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_salinity_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_salinity_uart(&session.device, &salinity);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_pressure_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_pressure_uart(&session.device, &pressure);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_read_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_response_uart(&session.device, output_config.enabled_mask, &reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("apply_updates", result);
    }

    printf("post_output_mask=%u\n", (unsigned)output_config.enabled_mask);
    printf("post_temperature_compensation_c=%.3f\n", temperature.temperature_c);
    printf("post_salinity_value=%.3f\n", salinity.value);
    printf("post_salinity_unit=%s\n", ezo_example_do_salinity_unit_name(salinity.unit));
    printf("post_pressure_kpa=%.3f\n", pressure.pressure_kpa);
    ezo_example_print_do_reading("post_", &reading);
  }

  ezo_example_close_uart(&session);
  return 0;
}
