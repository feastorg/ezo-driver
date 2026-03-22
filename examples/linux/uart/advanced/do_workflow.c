/*
Purpose: inspect D.O. compensation and output state, with optional setter examples.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is a D.O. circuit and response-code mode can be bootstrapped.
Next: read ../typed/read_do.c for the bare minimum read path.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_do.h"

#include <stdio.h>

static const char *salinity_unit_name(ezo_do_salinity_unit_t unit) {
  return unit == EZO_DO_SALINITY_UNIT_PPT ? "ppt" : "us_cm";
}

static void print_reading(const char *prefix, const ezo_do_reading_t *reading) {
  if ((reading->present_mask & EZO_DO_OUTPUT_MG_L) != 0U) {
    printf("%smilligrams_per_liter=%.3f\n", prefix, reading->milligrams_per_liter);
  }
  if ((reading->present_mask & EZO_DO_OUTPUT_PERCENT_SATURATION) != 0U) {
    printf("%spercent_saturation=%.3f\n", prefix, reading->percent_saturation);
  }
}

int main(int argc, char **argv) {
  const double planned_temperature_c = 25.0;
  const double planned_salinity_ppt = 35.0;
  const double planned_pressure_kpa = 101.325;
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_do_output_config_t output_config;
  ezo_do_temperature_compensation_t temperature;
  ezo_do_salinity_compensation_t salinity;
  ezo_do_pressure_compensation_t pressure;
  ezo_do_calibration_status_t calibration;
  ezo_do_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;

  if (!ezo_example_parse_uart_options(argc,
                                      argv,
                                      EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE,
                                      &options,
                                      &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [baud] [--apply]\n", argv[0]);
    return 1;
  }
  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");

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
  printf("current_salinity_unit=%s\n", salinity_unit_name(salinity.unit));
  printf("current_pressure_kpa=%.3f\n", pressure.pressure_kpa);
  printf("current_calibration_level=%u\n", (unsigned)calibration.level);
  print_reading("current_", &reading);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", planned_temperature_c);
  printf("planned_salinity_ppt=%.3f\n", planned_salinity_ppt);
  printf("planned_pressure_kpa=%.3f\n", planned_pressure_kpa);
  printf("planned_percent_saturation_output=%s\n", ezo_example_bool_name(1));

  if (apply_requested) {
    result = ezo_do_send_temperature_set_uart(&session.device, planned_temperature_c, 2, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_salinity_set_uart(&session.device,
                                             planned_salinity_ppt,
                                             EZO_DO_SALINITY_UNIT_PPT,
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
                                           1,
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
    printf("post_salinity_unit=%s\n", salinity_unit_name(salinity.unit));
    printf("post_pressure_kpa=%.3f\n", pressure.pressure_kpa);
    print_reading("post_", &reading);
  }

  ezo_example_close_uart(&session);
  return 0;
}
