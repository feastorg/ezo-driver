/*
Purpose: inspect ORP reading, calibration state, and extended-scale configuration, with an optional ORPext apply path.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is an ORP circuit and response-code mode can be bootstrapped.
Next: read orp_calibration.c for the staged 225 mV reference workflow.
*/

#include "example_base.h"
#include "example_products.h"
#include "example_uart.h"

#include "ezo_orp.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_orp_extended_scale_t planned_extended_scale = EZO_ORP_EXTENDED_SCALE_ENABLED;
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_orp_reading_t reading;
  ezo_orp_calibration_status_t calibration;
  ezo_orp_extended_scale_status_t extended_scale;
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
            "usage: %s [device_path] [baud] [--set-extended-scale=0|1] [--apply]\n",
            argv[0]);
    return 1;
  }
  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-extended-scale=");
  if (value != NULL) {
    uint32_t enabled = 0;

    if (!ezo_example_parse_uint32_arg(value, &enabled) || enabled > 1U) {
      fprintf(stderr, "invalid --set-extended-scale value\n");
      return 1;
    }
    planned_extended_scale = enabled != 0U ? EZO_ORP_EXTENDED_SCALE_ENABLED
                                           : EZO_ORP_EXTENDED_SCALE_DISABLED;
  }

  result = ezo_example_open_uart(options.device_path, options.baud, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_uart", result);
  }

  result = ezo_example_uart_bootstrap_response_codes(&session.device, EZO_PRODUCT_ORP, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_orp_send_read_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_orp_read_response_uart(&session.device, &reading);
  }
  if (result == EZO_OK) {
    result = ezo_orp_send_calibration_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_orp_read_calibration_status_uart(&session.device, &calibration);
  }
  if (result == EZO_OK) {
    result = ezo_orp_send_extended_scale_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_orp_read_extended_scale_uart(&session.device, &extended_scale);
  }
  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("query_current_state", result);
  }

  printf("transport=uart\n");
  printf("product=ORP\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("current_reading_mv=%.3f\n", reading.millivolts);
  printf("current_calibrated=%u\n", (unsigned)calibration.calibrated);
  printf("current_extended_scale=%s\n",
         ezo_example_bool_name(extended_scale.enabled == EZO_ORP_EXTENDED_SCALE_ENABLED));
  printf("vendor_note_extended_scale_changes_accuracy=1\n");
  printf("vendor_note_extended_scale_requires_5v=1\n");
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_extended_scale=%s\n",
         ezo_example_bool_name(planned_extended_scale == EZO_ORP_EXTENDED_SCALE_ENABLED));

  if (apply_requested) {
    result = ezo_orp_send_extended_scale_set_uart(&session.device, planned_extended_scale, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_orp_send_extended_scale_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_orp_read_extended_scale_uart(&session.device, &extended_scale);
    }
    if (result == EZO_OK) {
      result = ezo_orp_send_read_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_orp_read_response_uart(&session.device, &reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("apply_extended_scale", result);
    }

    printf("post_extended_scale=%s\n",
           ezo_example_bool_name(extended_scale.enabled == EZO_ORP_EXTENDED_SCALE_ENABLED));
    printf("post_reading_mv=%.3f\n", reading.millivolts);
  }

  ezo_example_close_uart(&session);
  return 0;
}
