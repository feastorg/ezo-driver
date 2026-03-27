/*
Purpose: stage ORP calibration steps with bounded preview reads and the vendor-standard 225 mV reference.
Defaults: /dev/i2c-1, address 98, step status, reference 225 mV.
Assumptions: the connected device is an ORP circuit in I2C mode and the probe is in a stable reference solution.
Next: read orp_workflow.c for extended-scale state and operational reads.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_orp.h"

#include <stdio.h>
#include <string.h>

typedef enum {
  STEP_STATUS = 0,
  STEP_CALIBRATE,
  STEP_CLEAR
} orp_step_t;

static int parse_step(const char *text, orp_step_t *step_out) {
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

static const char *step_name(orp_step_t step) {
  switch (step) {
    case STEP_CALIBRATE:
      return "calibrate";
    case STEP_CLEAR:
      return "clear";
    case STEP_STATUS:
    default:
      return "status";
  }
}

static ezo_result_t preview_readings(ezo_i2c_device_t *device,
                                     uint32_t preview_samples,
                                     uint32_t preview_interval_ms) {
  ezo_timing_hint_t hint;
  ezo_orp_reading_t reading;
  ezo_result_t result = EZO_OK;
  uint32_t index = 0;

  for (index = 0; index < preview_samples; ++index) {
    result = ezo_orp_send_read_i2c(device, &hint);
    if (result != EZO_OK) {
      return result;
    }
    ezo_example_wait_hint(&hint);
    result = ezo_orp_read_response_i2c(device, &reading);
    if (result != EZO_OK) {
      return result;
    }

    printf("preview_reading_%u_mv=%.3f\n", (unsigned)index, reading.millivolts);
    if (index + 1U < preview_samples) {
      ezo_example_sleep_ms(preview_interval_ms);
    }
  }

  return EZO_OK;
}

int main(int argc, char **argv) {
  orp_step_t step = STEP_STATUS;
  double reference_mv = 225.0;
  uint32_t preview_samples = 5U;
  uint32_t preview_interval_ms = 1000U;
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_orp_calibration_status_t calibration;
  ezo_orp_extended_scale_status_t extended_scale;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_i2c_options(argc, argv, 98U, &options, &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [address] [--step=status|calibrate|clear] "
            "[--reference-mv=225] [--preview-samples=5] [--preview-interval-ms=1000] "
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

  value = ezo_example_find_option_value(argc, argv, next_arg, "--reference-mv=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &reference_mv)) {
    fprintf(stderr, "invalid --reference-mv value\n");
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

  result = ezo_orp_send_calibration_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_orp_read_calibration_status_i2c(&session.device, &calibration);
  }
  if (result == EZO_OK) {
    result = ezo_orp_send_extended_scale_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_orp_read_extended_scale_i2c(&session.device, &extended_scale);
  }
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_status", result);
  }

  printf("transport=i2c\n");
  printf("product=ORP\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("step=%s\n", step_name(step));
  printf("current_calibrated=%u\n", (unsigned)calibration.calibrated);
  printf("current_extended_scale=%s\n",
         ezo_example_bool_name(extended_scale.enabled == EZO_ORP_EXTENDED_SCALE_ENABLED));
  printf("vendor_guidance_default_reference_mv=225.000\n");
  printf("vendor_guidance_wait_for_stable_reference=1\n");
  printf("preview_samples=%u\n", (unsigned)preview_samples);
  printf("preview_interval_ms=%u\n", (unsigned)preview_interval_ms);
  printf("apply_requested=%d\n", apply_requested);
  if (step == STEP_CALIBRATE) {
    printf("planned_reference_mv=%.3f\n", reference_mv);
  }

  if (step == STEP_CALIBRATE) {
    result = preview_readings(&session.device, preview_samples, preview_interval_ms);
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("preview_readings", result);
    }
  }

  if (apply_requested) {
    if (step == STEP_CLEAR) {
      result = ezo_orp_send_clear_calibration_i2c(&session.device, &hint);
    } else if (step == STEP_CALIBRATE) {
      result = ezo_orp_send_calibration_i2c(&session.device, reference_mv, 0, &hint);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("send_calibration", result);
    }

    if (step != STEP_STATUS) {
      ezo_example_wait_hint(&hint);
      result = ezo_orp_send_calibration_query_i2c(&session.device, &hint);
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_orp_read_calibration_status_i2c(&session.device, &calibration);
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
