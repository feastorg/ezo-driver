/*
Purpose: stage pH calibration steps with bounded preview reads and explicit vendor ordering notes.
Defaults: /dev/i2c-1, address 99, step status, midpoint reference 7.00.
Assumptions: the connected device is a pH circuit in I2C mode and the probe is in the matching calibration solution.
Next: read ph_workflow.c for operational temperature-compensation and range state.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_ph.h"

#include <stdio.h>
#include <string.h>

typedef enum {
  STEP_STATUS = 0,
  STEP_MID,
  STEP_LOW,
  STEP_HIGH,
  STEP_CLEAR
} ph_step_t;

static int parse_step(const char *text, ph_step_t *step_out) {
  if (text == NULL || step_out == NULL) {
    return 0;
  }
  if (strcmp(text, "status") == 0) {
    *step_out = STEP_STATUS;
    return 1;
  }
  if (strcmp(text, "mid") == 0) {
    *step_out = STEP_MID;
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
  return 0;
}

static const char *step_name(ph_step_t step) {
  switch (step) {
    case STEP_MID:
      return "mid";
    case STEP_LOW:
      return "low";
    case STEP_HIGH:
      return "high";
    case STEP_CLEAR:
      return "clear";
    case STEP_STATUS:
    default:
      return "status";
  }
}

static ezo_ph_calibration_point_t calibration_point_for_step(ph_step_t step) {
  switch (step) {
    case STEP_LOW:
      return EZO_PH_CALIBRATION_POINT_LOW;
    case STEP_HIGH:
      return EZO_PH_CALIBRATION_POINT_HIGH;
    case STEP_MID:
    case STEP_CLEAR:
    case STEP_STATUS:
    default:
      return EZO_PH_CALIBRATION_POINT_MID;
  }
}

static ezo_result_t preview_readings(ezo_i2c_device_t *device,
                                     uint32_t preview_samples,
                                     uint32_t preview_interval_ms) {
  ezo_timing_hint_t hint;
  ezo_ph_reading_t reading;
  ezo_result_t result = EZO_OK;
  uint32_t index = 0;

  for (index = 0; index < preview_samples; ++index) {
    result = ezo_ph_send_read_i2c(device, &hint);
    if (result != EZO_OK) {
      return result;
    }
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_response_i2c(device, &reading);
    if (result != EZO_OK) {
      return result;
    }

    printf("preview_reading_%u_ph=%.3f\n", (unsigned)index, reading.ph);
    if (index + 1U < preview_samples) {
      ezo_example_sleep_ms(preview_interval_ms);
    }
  }

  return EZO_OK;
}

int main(int argc, char **argv) {
  ph_step_t step = STEP_STATUS;
  double reference_ph = 7.00;
  uint32_t preview_samples = 5U;
  uint32_t preview_interval_ms = 1000U;
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_ph_temperature_compensation_t temperature;
  ezo_ph_calibration_status_t calibration;
  ezo_ph_slope_t slope;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_i2c_options(argc, argv, 99U, &options, &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [address] [--step=status|mid|low|high|clear] "
            "[--reference-ph=7.00] [--preview-samples=5] [--preview-interval-ms=1000] "
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

  value = ezo_example_find_option_value(argc, argv, next_arg, "--reference-ph=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &reference_ph)) {
    fprintf(stderr, "invalid --reference-ph value\n");
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

  result = ezo_ph_send_temperature_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_temperature_i2c(&session.device, &temperature);
  }
  if (result == EZO_OK) {
    result = ezo_ph_send_calibration_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_calibration_status_i2c(&session.device, &calibration);
  }
  if (result == EZO_OK) {
    result = ezo_ph_send_slope_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_slope_i2c(&session.device, &slope);
  }
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_status", result);
  }

  printf("transport=i2c\n");
  printf("product=pH\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("step=%s\n", step_name(step));
  printf("current_temperature_compensation_c=%.3f\n", temperature.temperature_c);
  printf("current_calibration_level=%s\n", ezo_example_ph_calibration_name(calibration.level));
  printf("current_slope_acid_percent=%.3f\n", slope.acid_percent);
  printf("current_slope_base_percent=%.3f\n", slope.base_percent);
  printf("current_slope_neutral_mv=%.3f\n", slope.neutral_mv);
  printf("vendor_guidance_midpoint_first=1\n");
  printf("vendor_guidance_midpoint_clears_other_points=1\n");
  printf("vendor_guidance_check_slope_after_calibration=1\n");
  printf("preview_samples=%u\n", (unsigned)preview_samples);
  printf("preview_interval_ms=%u\n", (unsigned)preview_interval_ms);
  printf("apply_requested=%d\n", apply_requested);
  if (step == STEP_MID || step == STEP_LOW || step == STEP_HIGH) {
    printf("planned_reference_ph=%.3f\n", reference_ph);
  }

  if (step != STEP_STATUS && step != STEP_CLEAR) {
    result = preview_readings(&session.device, preview_samples, preview_interval_ms);
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("preview_readings", result);
    }
  }

  if (apply_requested) {
    if (step == STEP_CLEAR) {
      result = ezo_ph_send_clear_calibration_i2c(&session.device, &hint);
    } else if (step != STEP_STATUS) {
      result = ezo_ph_send_calibration_i2c(&session.device,
                                           calibration_point_for_step(step),
                                           reference_ph,
                                           2,
                                           &hint);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("send_calibration", result);
    }

    if (step != STEP_STATUS) {
      ezo_example_wait_hint(&hint);

      result = ezo_ph_send_calibration_query_i2c(&session.device, &hint);
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_ph_read_calibration_status_i2c(&session.device, &calibration);
      }
      if (result == EZO_OK) {
        result = ezo_ph_send_slope_query_i2c(&session.device, &hint);
      }
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_ph_read_slope_i2c(&session.device, &slope);
      }
      if (result != EZO_OK) {
        ezo_example_close_i2c(&session);
        return ezo_example_print_error("post_calibration_query", result);
      }

      printf("post_calibration_level=%s\n", ezo_example_ph_calibration_name(calibration.level));
      printf("post_slope_acid_percent=%.3f\n", slope.acid_percent);
      printf("post_slope_base_percent=%.3f\n", slope.base_percent);
      printf("post_slope_neutral_mv=%.3f\n", slope.neutral_mv);
    }
  }

  ezo_example_close_i2c(&session);
  return 0;
}
