/*
Purpose: inspect pH temperature, calibration, slope, and range state, with optional setters.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is a pH circuit and response-code mode can be bootstrapped.
Next: read calibration_transfer.c for export/import workflows.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_ph.h"

#include <stdio.h>

static const char *calibration_name(ezo_ph_calibration_level_t level) {
  switch (level) {
    case EZO_PH_CALIBRATION_NONE:
      return "none";
    case EZO_PH_CALIBRATION_ONE_POINT:
      return "one_point";
    case EZO_PH_CALIBRATION_TWO_POINT:
      return "two_point";
    case EZO_PH_CALIBRATION_THREE_POINT:
      return "three_point";
    default:
      return "unknown";
  }
}

int main(int argc, char **argv) {
  const double planned_temperature_c = 25.0;
  const ezo_ph_extended_range_t planned_extended_range = EZO_PH_EXTENDED_RANGE_ENABLED;
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_ph_temperature_compensation_t temperature;
  ezo_ph_calibration_status_t calibration;
  ezo_ph_slope_t slope;
  ezo_ph_extended_range_status_t extended_range;
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

  result = ezo_example_uart_bootstrap_response_codes(&session.device, EZO_PRODUCT_PH, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_ph_send_temperature_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_temperature_uart(&session.device, &temperature);
  }
  if (result == EZO_OK) {
    result = ezo_ph_send_calibration_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_calibration_status_uart(&session.device, &calibration);
  }
  if (result == EZO_OK) {
    result = ezo_ph_send_slope_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_slope_uart(&session.device, &slope);
  }
  if (result == EZO_OK) {
    result = ezo_ph_send_extended_range_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_extended_range_uart(&session.device, &extended_range);
  }

  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("query_current_state", result);
  }

  printf("transport=uart\n");
  printf("product=pH\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("current_temperature_compensation_c=%.3f\n", temperature.temperature_c);
  printf("current_calibration_level=%s\n", calibration_name(calibration.level));
  printf("current_slope_acid_percent=%.3f\n", slope.acid_percent);
  printf("current_slope_base_percent=%.3f\n", slope.base_percent);
  printf("current_slope_neutral_mv=%.3f\n", slope.neutral_mv);
  printf("current_extended_range=%s\n",
         ezo_example_bool_name(extended_range.enabled == EZO_PH_EXTENDED_RANGE_ENABLED));
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", planned_temperature_c);
  printf("planned_extended_range=%s\n", ezo_example_bool_name(1));

  if (apply_requested) {
    result = ezo_ph_send_temperature_set_uart(&session.device, planned_temperature_c, 2, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_ph_send_extended_range_set_uart(&session.device, planned_extended_range, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_ph_send_temperature_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_read_temperature_uart(&session.device, &temperature);
    }
    if (result == EZO_OK) {
      result = ezo_ph_send_extended_range_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_read_extended_range_uart(&session.device, &extended_range);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("apply_updates", result);
    }

    printf("post_temperature_compensation_c=%.3f\n", temperature.temperature_c);
    printf("post_extended_range=%s\n",
           ezo_example_bool_name(extended_range.enabled == EZO_PH_EXTENDED_RANGE_ENABLED));
  }

  ezo_example_close_uart(&session);
  return 0;
}
