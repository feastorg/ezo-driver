/*
Purpose: stage D.O. calibration steps with bounded preview reads and explicit low/high ordering notes.
Defaults: /dev/ttyUSB0 at 9600 baud, step status, and operating recalibration temperature 25.0 C.
Assumptions: the connected device is a D.O. circuit, response-code mode can be bootstrapped, and the probe is in the matching zero or atmospheric calibration condition.
Next: read do_workflow.c for operational compensation and output state, or do_full_compensation_chain.c for the full chain example.
*/

#include "example_base.h"
#include "example_products.h"
#include "example_uart.h"

#include "ezo_do.h"

#include <stdio.h>
#include <string.h>

typedef enum {
  STEP_STATUS = 0,
  STEP_LOW,
  STEP_HIGH,
  STEP_CLEAR,
  STEP_OPERATING_TEMP_RECAL
} do_step_t;

static int parse_step(const char *text, do_step_t *step_out) {
  if (text == NULL || step_out == NULL) {
    return 0;
  }
  if (strcmp(text, "status") == 0) {
    *step_out = STEP_STATUS;
    return 1;
  }
  if (strcmp(text, "low") == 0) {
    *step_out = STEP_LOW;
    return 1;
  }
  if (strcmp(text, "high") == 0) {
    *step_out = STEP_HIGH;
    return 1;
  }
  if (strcmp(text, "clear") == 0) {
    *step_out = STEP_CLEAR;
    return 1;
  }
  if (strcmp(text, "operating-temp-recal") == 0) {
    *step_out = STEP_OPERATING_TEMP_RECAL;
    return 1;
  }
  return 0;
}

static ezo_do_calibration_point_t calibration_point_for_step(do_step_t step) {
  return step == STEP_LOW ? EZO_DO_CALIBRATION_ZERO : EZO_DO_CALIBRATION_ATMOSPHERIC;
}

static ezo_result_t preview_readings(ezo_uart_device_t *device,
                                     ezo_do_output_mask_t output_mask,
                                     uint32_t preview_samples,
                                     uint32_t preview_interval_ms) {
  ezo_timing_hint_t hint;
  ezo_do_reading_t reading;
  ezo_result_t result = EZO_OK;
  uint32_t index = 0;

  for (index = 0; index < preview_samples; ++index) {
    result = ezo_do_send_read_uart(device, &hint);
    if (result != EZO_OK) {
      return result;
    }
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_response_uart(device, output_mask, &reading);
    if (result != EZO_OK) {
      return result;
    }

    ezo_example_print_do_reading("preview_", &reading);
    if (index + 1U < preview_samples) {
      ezo_example_sleep_ms(preview_interval_ms);
    }
  }

  return EZO_OK;
}

int main(int argc, char **argv) {
  do_step_t step = STEP_STATUS;
  double operating_temperature_c = 25.0;
  uint32_t preview_samples = 5U;
  uint32_t preview_interval_ms = 1000U;
  ezo_example_uart_options_t options;
  ezo_example_uart_session_t session;
  ezo_timing_hint_t hint;
  ezo_do_output_config_t output_config;
  ezo_do_temperature_compensation_t temperature;
  ezo_do_salinity_compensation_t salinity;
  ezo_do_pressure_compensation_t pressure;
  ezo_do_calibration_status_t calibration;
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
            "usage: %s [device_path] [baud] [--step=status|low|high|clear|operating-temp-recal] "
            "[--temperature-c=25.0] [--preview-samples=5] [--preview-interval-ms=1000] "
            "[--apply]\n",
            argv[0]);
    return 1;
  }
  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--step=");
  if (value != NULL && !parse_step(value, &step)) {
    fprintf(stderr, "invalid --step value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--temperature-c=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &operating_temperature_c)) {
    fprintf(stderr, "invalid --temperature-c value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--preview-samples=");
  if (value != NULL && !ezo_example_parse_uint32_arg(value, &preview_samples)) {
    fprintf(stderr, "invalid --preview-samples value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--preview-interval-ms=");
  if (value != NULL && !ezo_example_parse_uint32_arg(value, &preview_interval_ms)) {
    fprintf(stderr, "invalid --preview-interval-ms value\n");
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
  if (result != EZO_OK) {
    ezo_example_close_uart(&session);
    return ezo_example_print_error("query_status", result);
  }

  printf("transport=uart\n");
  printf("product=D.O.\n");
  printf("device_path=%s\n", options.device_path);
  printf("baud_rate=%u\n", (unsigned)options.baud_rate);
  printf("step=%s\n",
         step == STEP_LOW ? "low" :
         step == STEP_HIGH ? "high" :
         step == STEP_CLEAR ? "clear" :
         step == STEP_OPERATING_TEMP_RECAL ? "operating-temp-recal" : "status");
  printf("current_output_mask=%u\n", (unsigned)output_config.enabled_mask);
  printf("current_temperature_compensation_c=%.3f\n", temperature.temperature_c);
  printf("current_salinity_value=%.3f\n", salinity.value);
  printf("current_salinity_unit=%s\n", ezo_example_do_salinity_unit_name(salinity.unit));
  printf("current_pressure_kpa=%.3f\n", pressure.pressure_kpa);
  printf("current_calibration_level=%u\n", (unsigned)calibration.level);
  printf("vendor_guidance_calibrate_before_compensating=1\n");
  printf("vendor_guidance_low_point_before_high_point=1\n");
  printf("vendor_guidance_default_compensation_values_required=1\n");
  printf("preview_samples=%u\n", (unsigned)preview_samples);
  printf("preview_interval_ms=%u\n", (unsigned)preview_interval_ms);
  printf("apply_requested=%d\n", apply_requested);
  if (step == STEP_OPERATING_TEMP_RECAL) {
    printf("planned_operating_temperature_c=%.3f\n", operating_temperature_c);
  }

  if (step != STEP_STATUS && step != STEP_CLEAR) {
    result = preview_readings(&session.device,
                              output_config.enabled_mask,
                              preview_samples,
                              preview_interval_ms);
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("preview_readings", result);
    }
  }

  if (apply_requested) {
    if (step == STEP_OPERATING_TEMP_RECAL) {
      result = ezo_do_send_temperature_set_uart(&session.device, operating_temperature_c, 2, &hint);
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_uart_read_ok(&session.device);
      }
    }
    if (result == EZO_OK && step == STEP_CLEAR) {
      result = ezo_do_send_clear_calibration_uart(&session.device, &hint);
    } else if (result == EZO_OK && step != STEP_STATUS) {
      result = ezo_do_send_calibration_uart(&session.device, calibration_point_for_step(step), &hint);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&session);
      return ezo_example_print_error("send_calibration", result);
    }

    if (step != STEP_STATUS) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&session.device);
      if (result == EZO_OK) {
        result = ezo_do_send_temperature_query_uart(&session.device, &hint);
      }
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_do_read_temperature_uart(&session.device, &temperature);
      }
      if (result == EZO_OK) {
        result = ezo_do_send_calibration_query_uart(&session.device, &hint);
      }
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_do_read_calibration_status_uart(&session.device, &calibration);
      }
      if (result != EZO_OK) {
        ezo_example_close_uart(&session);
        return ezo_example_print_error("post_calibration_query", result);
      }

      printf("post_temperature_compensation_c=%.3f\n", temperature.temperature_c);
      printf("post_calibration_level=%u\n", (unsigned)calibration.level);
    }
  }

  ezo_example_close_uart(&session);
  return 0;
}
