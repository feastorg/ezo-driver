/*
Purpose: stage RTD one-point offset calibration with bounded preview reads and explicit reference temperature.
Defaults: /dev/i2c-1, address 102, step status, reference 100.0.
Assumptions: the connected device is an RTD circuit in I2C mode and the probe is at a stable known temperature.
Next: read rtd_workflow.c for logger, scale, and memory-state operations.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_rtd.h"

#include <stdio.h>
#include <string.h>

typedef enum {
  STEP_STATUS = 0,
  STEP_CALIBRATE,
  STEP_CLEAR
} rtd_step_t;

static int parse_step(const char *text, rtd_step_t *step_out) {
  if (text == NULL || step_out == NULL) {
    return 0;
  }
  if (strcmp(text, "status") == 0) {
    *step_out = STEP_STATUS;
    return 1;
  }
  if (strcmp(text, "calibrate") == 0) {
    *step_out = STEP_CALIBRATE;
    return 1;
  }
  if (strcmp(text, "clear") == 0) {
    *step_out = STEP_CLEAR;
    return 1;
  }
  return 0;
}

static ezo_result_t preview_readings(ezo_i2c_device_t *device,
                                     ezo_rtd_scale_t scale,
                                     uint32_t preview_samples,
                                     uint32_t preview_interval_ms) {
  ezo_timing_hint_t hint;
  ezo_rtd_reading_t reading;
  ezo_result_t result = EZO_OK;
  uint32_t index = 0;

  for (index = 0; index < preview_samples; ++index) {
    result = ezo_rtd_send_read_i2c(device, &hint);
    if (result != EZO_OK) {
      return result;
    }
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_response_i2c(device, scale, &reading);
    if (result != EZO_OK) {
      return result;
    }

    printf("preview_reading_%u_temperature=%.3f\n", (unsigned)index, reading.temperature);
    printf("preview_reading_%u_scale=%s\n", (unsigned)index, ezo_example_rtd_scale_name(reading.scale));
    if (index + 1U < preview_samples) {
      ezo_example_sleep_ms(preview_interval_ms);
    }
  }

  return EZO_OK;
}

int main(int argc, char **argv) {
  rtd_step_t step = STEP_STATUS;
  double reference_temperature = 100.0;
  uint32_t preview_samples = 5U;
  uint32_t preview_interval_ms = 1000U;
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t scale;
  ezo_rtd_calibration_status_t calibration;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_i2c_options(argc, argv, 102U, &options, &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [address] [--step=status|calibrate|clear] "
            "[--reference-temp=100.0] [--preview-samples=5] [--preview-interval-ms=1000] "
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

  value = ezo_example_find_option_value(argc, argv, next_arg, "--reference-temp=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &reference_temperature)) {
    fprintf(stderr, "invalid --reference-temp value\n");
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

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_rtd_send_scale_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_scale_i2c(&session.device, &scale);
  }
  if (result == EZO_OK) {
    result = ezo_rtd_send_calibration_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_calibration_status_i2c(&session.device, &calibration);
  }
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_status", result);
  }

  printf("transport=i2c\n");
  printf("product=RTD\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("step=%s\n", step == STEP_CALIBRATE ? "calibrate" : (step == STEP_CLEAR ? "clear" : "status"));
  printf("current_scale=%s\n", ezo_example_rtd_scale_name(scale.scale));
  printf("current_calibrated=%u\n", (unsigned)calibration.calibrated);
  printf("preview_samples=%u\n", (unsigned)preview_samples);
  printf("preview_interval_ms=%u\n", (unsigned)preview_interval_ms);
  printf("apply_requested=%d\n", apply_requested);
  if (step == STEP_CALIBRATE) {
    printf("planned_reference_temp=%.3f\n", reference_temperature);
  }

  if (step == STEP_CALIBRATE) {
    result = preview_readings(&session.device, scale.scale, preview_samples, preview_interval_ms);
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("preview_readings", result);
    }
  }

  if (apply_requested) {
    if (step == STEP_CLEAR) {
      result = ezo_rtd_send_clear_calibration_i2c(&session.device, &hint);
    } else if (step == STEP_CALIBRATE) {
      result = ezo_rtd_send_calibration_i2c(&session.device, reference_temperature, 2, &hint);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("send_calibration", result);
    }

    if (step != STEP_STATUS) {
      ezo_example_wait_hint(&hint);
      result = ezo_rtd_send_calibration_query_i2c(&session.device, &hint);
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_rtd_read_calibration_status_i2c(&session.device, &calibration);
      }
      if (result != EZO_OK) {
        ezo_example_close_i2c(&session);
        return ezo_example_print_error("post_calibration_query", result);
      }

      printf("post_calibrated=%u\n", (unsigned)calibration.calibrated);
    }
  }

  ezo_example_close_i2c(&session);
  return 0;
}
