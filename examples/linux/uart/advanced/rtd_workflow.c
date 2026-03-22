/*
Purpose: inspect RTD scale, logger, and memory state, with optional setter examples.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is an RTD circuit and response-code mode can be bootstrapped.
Next: read ../typed/read_rtd.c for the smallest possible read path.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_rtd.h"

#include <stdio.h>

enum {
  RTD_MEMORY_CAPACITY = 64
};

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

int main(int argc, char **argv) {
  const ezo_rtd_scale_t planned_scale = EZO_RTD_SCALE_FAHRENHEIT;
  const uint32_t planned_logger_interval = 60U;
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t scale;
  ezo_rtd_calibration_status_t calibration;
  ezo_rtd_logger_status_t logger;
  ezo_rtd_memory_status_t memory;
  ezo_rtd_memory_value_t values[RTD_MEMORY_CAPACITY];
  size_t value_count = 0;
  size_t index = 0;
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

  result = ezo_example_uart_bootstrap_response_codes(&session.device, EZO_PRODUCT_RTD, NULL, NULL);
  if (result == EZO_OK) {
    result = ezo_rtd_send_scale_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_scale_uart(&session.device, &scale);
  }
  if (result == EZO_OK) {
    result = ezo_rtd_send_calibration_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_calibration_status_uart(&session.device, &calibration);
  }
  if (result == EZO_OK) {
    result = ezo_rtd_send_logger_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_logger_uart(&session.device, &logger);
  }
  if (result == EZO_OK) {
    result = ezo_rtd_send_memory_query_uart(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_memory_status_uart(&session.device, &memory);
  }
  if (result == EZO_OK && memory.last_index > 0U && memory.last_index <= RTD_MEMORY_CAPACITY) {
    result = ezo_rtd_send_memory_all_uart(&session.device, &hint);
  }
  if (result == EZO_OK && memory.last_index > 0U && memory.last_index <= RTD_MEMORY_CAPACITY) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_memory_all_uart(&session.device,
                                          scale.scale,
                                          values,
                                          RTD_MEMORY_CAPACITY,
                                          &value_count);
  }

  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("query_current_state", result);
  }

  printf("transport=uart\n");
  printf("product=RTD\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("current_scale=%s\n", scale_name(scale.scale));
  printf("current_calibrated=%u\n", (unsigned)calibration.calibrated);
  printf("current_logger_interval_units=%u\n", (unsigned)logger.interval_units);
  printf("current_memory_last_index=%u\n", (unsigned)memory.last_index);
  if (memory.last_index > RTD_MEMORY_CAPACITY) {
    printf("memory_value_count=0\n");
    printf("memory_dump_skipped=1\n");
  } else {
    printf("memory_value_count=%u\n", (unsigned)value_count);
    for (index = 0; index < value_count; ++index) {
      printf("memory_value_%u=%.3f\n", (unsigned)index, values[index].temperature);
    }
  }
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_scale=%s\n", scale_name(planned_scale));
  printf("planned_logger_interval_units=%u\n", (unsigned)planned_logger_interval);

  if (apply_requested) {
    result = ezo_rtd_send_scale_set_uart(&session.device, planned_scale, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_rtd_send_logger_set_uart(&session.device, planned_logger_interval, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
    }
    if (result == EZO_OK) {
      result = ezo_rtd_send_scale_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_rtd_read_scale_uart(&session.device, &scale);
    }
    if (result == EZO_OK) {
      result = ezo_rtd_send_logger_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_rtd_read_logger_uart(&session.device, &logger);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("apply_updates", result);
    }

    printf("post_scale=%s\n", scale_name(scale.scale));
    printf("post_logger_interval_units=%u\n", (unsigned)logger.interval_units);
  }

  ezo_example_close_uart(&session);
  return 0;
}
