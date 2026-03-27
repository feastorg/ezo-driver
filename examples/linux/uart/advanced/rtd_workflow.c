/*
Purpose: inspect RTD scale, calibration, logger, and memory state, including guarded recall and clear-memory flows.
Defaults: /dev/ttyUSB0 at 9600 baud.
Assumptions: the connected device is an RTD circuit and response-code mode can be bootstrapped.
Next: read rtd_calibration.c for the staged one-point offset calibration example.
*/

#include "example_base.h"
#include "example_products.h"
#include "example_uart.h"

#include "ezo_rtd.h"

#include <stdio.h>
#include <string.h>

enum {
  RTD_MEMORY_CAPACITY = 64,
  RTD_MEMORY_PREVIEW_LIMIT = 5
};

static int parse_scale_arg(const char *text, ezo_rtd_scale_t *scale_out) {
  if (text == NULL || scale_out == NULL) {
    return 0;
  }
  if (strcmp(text, "c") == 0 || strcmp(text, "celsius") == 0) {
    *scale_out = EZO_RTD_SCALE_CELSIUS;
    return 1;
  }
  if (strcmp(text, "k") == 0 || strcmp(text, "kelvin") == 0) {
    *scale_out = EZO_RTD_SCALE_KELVIN;
    return 1;
  }
  if (strcmp(text, "f") == 0 || strcmp(text, "fahrenheit") == 0) {
    *scale_out = EZO_RTD_SCALE_FAHRENHEIT;
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  ezo_rtd_scale_t planned_scale = EZO_RTD_SCALE_FAHRENHEIT;
  uint32_t planned_logger_interval = 60U;
  uint32_t sequential_count = RTD_MEMORY_PREVIEW_LIMIT;
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t scale;
  ezo_rtd_calibration_status_t calibration;
  ezo_rtd_logger_status_t logger;
  ezo_rtd_memory_status_t memory;
  ezo_rtd_memory_value_t values[RTD_MEMORY_CAPACITY];
  ezo_rtd_memory_entry_t sequential_entry;
  size_t value_count = 0;
  size_t index = 0;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  int clear_memory_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_uart_options(argc,
                                      argv,
                                      EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE,
                                      &options,
                                      &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [baud] [--set-scale=celsius|kelvin|fahrenheit] "
            "[--set-logger-interval=60] [--sequential-count=5] [--clear-memory] [--apply]\n",
            argv[0]);
    return 1;
  }

  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");
  clear_memory_requested = ezo_example_has_flag(argc, argv, next_arg, "--clear-memory");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-scale=");
  if (value != NULL && !parse_scale_arg(value, &planned_scale)) {
    fprintf(stderr, "invalid --set-scale value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-logger-interval=");
  if (value != NULL && !ezo_example_parse_uint32_arg(value, &planned_logger_interval)) {
    fprintf(stderr, "invalid --set-logger-interval value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--sequential-count=");
  if (value != NULL && !ezo_example_parse_uint32_arg(value, &sequential_count)) {
    fprintf(stderr, "invalid --sequential-count value\n");
    return 1;
  }
  if (sequential_count > RTD_MEMORY_PREVIEW_LIMIT) {
    sequential_count = RTD_MEMORY_PREVIEW_LIMIT;
  }

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
  if (result == EZO_OK && logger.interval_units == 0U && memory.last_index > 0U &&
      memory.last_index <= RTD_MEMORY_CAPACITY) {
    result = ezo_rtd_send_memory_all_uart(&session.device, &hint);
  }
  if (result == EZO_OK && logger.interval_units == 0U && memory.last_index > 0U &&
      memory.last_index <= RTD_MEMORY_CAPACITY) {
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
  printf("current_scale=%s\n", ezo_example_rtd_scale_name(scale.scale));
  printf("current_calibrated=%u\n", (unsigned)calibration.calibrated);
  printf("current_logger_interval_units=%u\n", (unsigned)logger.interval_units);
  printf("current_memory_last_index=%u\n", (unsigned)memory.last_index);
  printf("memory_recall_requires_logger_disabled=1\n");
  printf("sequential_preview_count=%u\n", (unsigned)sequential_count);
  if (logger.interval_units != 0U) {
    printf("memory_recall_skipped_reason=logger_enabled\n");
  } else if (memory.last_index > RTD_MEMORY_CAPACITY) {
    printf("memory_recall_skipped_reason=memory_capacity_exceeded\n");
  } else {
    printf("memory_value_count=%u\n", (unsigned)value_count);
    for (index = 0; index < value_count; ++index) {
      printf("memory_value_%u=%.3f\n", (unsigned)index, values[index].temperature);
    }

    for (index = 0; index < sequential_count && index < memory.last_index; ++index) {
      result = ezo_rtd_send_memory_next_uart(&session.device, &hint);
      if (result != EZO_OK) {
        ezo_example_close_uart(&session);
        return ezo_example_print_error("send_memory_next", result);
      }
      ezo_example_wait_hint(&hint);
      result = ezo_rtd_read_memory_entry_uart(&session.device, scale.scale, &sequential_entry);
      if (result != EZO_OK) {
        ezo_example_close_uart(&session);
        return ezo_example_print_error("read_memory_next", result);
      }

      printf("sequential_memory_index_%u=%u\n",
             (unsigned)index,
             (unsigned)sequential_entry.index);
      printf("sequential_memory_value_%u=%.3f\n",
             (unsigned)index,
             sequential_entry.temperature);
    }
  }
  printf("apply_requested=%d\n", apply_requested);
  printf("clear_memory_requested=%d\n", clear_memory_requested);
  printf("planned_scale=%s\n", ezo_example_rtd_scale_name(planned_scale));
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
    if (result == EZO_OK && clear_memory_requested) {
      result = ezo_rtd_send_memory_clear_uart(&session.device, &hint);
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_uart_read_ok(&session.device);
      }
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
    if (result == EZO_OK) {
      result = ezo_rtd_send_memory_query_uart(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_rtd_read_memory_status_uart(&session.device, &memory);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("apply_updates", result);
    }

    printf("post_scale=%s\n", ezo_example_rtd_scale_name(scale.scale));
    printf("post_logger_interval_units=%u\n", (unsigned)logger.interval_units);
    printf("post_memory_last_index=%u\n", (unsigned)memory.last_index);
  }

  ezo_example_close_uart(&session);
  return 0;
}
