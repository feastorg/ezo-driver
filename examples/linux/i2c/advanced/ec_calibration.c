/*
Purpose: stage EC calibration steps with bounded preview reads, dry-first guidance, and optional probe-K setup.
Defaults: /dev/i2c-1, address 100, step status, and common reference defaults for single/low/high points.
Assumptions: the connected device is an EC circuit in I2C mode and the probe is in the matching calibration state or solution.
Next: read ec_workflow.c for output, probe-K, TDS, and one-shot RT,n operational state.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_ec.h"

#include <stdio.h>
#include <string.h>

typedef enum {
  STEP_STATUS = 0,
  STEP_DRY,
  STEP_SINGLE,
  STEP_LOW,
  STEP_HIGH,
  STEP_CLEAR
} ec_step_t;

static int parse_step(const char *text, ec_step_t *step_out) {
  if (text == NULL || step_out == NULL) {
    return 0;
  }
  if (strcmp(text, "status") == 0) {
    *step_out = STEP_STATUS;
    return 1;
  }
  if (strcmp(text, "dry") == 0) {
    *step_out = STEP_DRY;
    return 1;
  }
  if (strcmp(text, "single") == 0) {
    *step_out = STEP_SINGLE;
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

static ezo_ec_calibration_point_t calibration_point_for_step(ec_step_t step) {
  switch (step) {
    case STEP_DRY:
      return EZO_EC_CALIBRATION_DRY;
    case STEP_LOW:
      return EZO_EC_CALIBRATION_LOW_POINT;
    case STEP_HIGH:
      return EZO_EC_CALIBRATION_HIGH_POINT;
    case STEP_SINGLE:
    case STEP_CLEAR:
    case STEP_STATUS:
    default:
      return EZO_EC_CALIBRATION_SINGLE_POINT;
  }
}

static double default_reference_for_step(ec_step_t step) {
  switch (step) {
    case STEP_LOW:
      return 84.0;
    case STEP_HIGH:
      return 12880.0;
    case STEP_SINGLE:
      return 1413.0;
    case STEP_DRY:
    case STEP_CLEAR:
    case STEP_STATUS:
    default:
      return 0.0;
  }
}

static const char *step_name(ec_step_t step) {
  switch (step) {
    case STEP_DRY:
      return "dry";
    case STEP_SINGLE:
      return "single";
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

static ezo_result_t preview_readings(ezo_i2c_device_t *device,
                                     ezo_ec_output_mask_t output_mask,
                                     uint32_t preview_samples,
                                     uint32_t preview_interval_ms) {
  ezo_timing_hint_t hint;
  ezo_ec_reading_t reading;
  ezo_result_t result = EZO_OK;
  uint32_t index = 0;

  for (index = 0; index < preview_samples; ++index) {
    result = ezo_ec_send_read_i2c(device, &hint);
    if (result != EZO_OK) {
      return result;
    }
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_i2c(device, output_mask, &reading);
    if (result != EZO_OK) {
      return result;
    }

    ezo_example_print_ec_reading("preview_", &reading);
    if (index + 1U < preview_samples) {
      ezo_example_sleep_ms(preview_interval_ms);
    }
  }

  return EZO_OK;
}

int main(int argc, char **argv) {
  ec_step_t step = STEP_STATUS;
  double reference_us = 0.0;
  double planned_probe_k = 1.0;
  int probe_k_specified = 0;
  uint32_t preview_samples = 5U;
  uint32_t preview_interval_ms = 1000U;
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_ec_output_config_t output_config;
  ezo_ec_temperature_compensation_t temperature;
  ezo_ec_probe_k_t probe_k;
  ezo_ec_calibration_status_t calibration;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  int reference_specified = 0;
  const char *value = NULL;

  if (!ezo_example_parse_i2c_options(argc, argv, 100U, &options, &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [address] [--step=status|dry|single|low|high|clear] "
            "[--reference-us=1413] [--probe-k=1.0] [--preview-samples=5] "
            "[--preview-interval-ms=1000] [--apply]\n",
            argv[0]);
    return 1;
  }
  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--step=");
  if (value != NULL && !parse_step(value, &step)) {
    fprintf(stderr, "invalid --step value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--reference-us=");
  if (value != NULL) {
    if (!ezo_example_parse_double_arg(value, &reference_us)) {
      fprintf(stderr, "invalid --reference-us value\n");
      return 1;
    }
    reference_specified = 1;
  }
  if (!reference_specified) {
    reference_us = default_reference_for_step(step);
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--probe-k=");
  if (value != NULL) {
    if (!ezo_example_parse_double_arg(value, &planned_probe_k)) {
      fprintf(stderr, "invalid --probe-k value\n");
      return 1;
    }
    probe_k_specified = 1;
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

  result = ezo_ec_send_output_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_output_config_i2c(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_temperature_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_temperature_i2c(&session.device, &temperature);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_probe_k_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_probe_k_i2c(&session.device, &probe_k);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_calibration_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_calibration_status_i2c(&session.device, &calibration);
  }
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_status", result);
  }

  printf("transport=i2c\n");
  printf("product=EC\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("step=%s\n", step_name(step));
  printf("current_output_mask=%u\n", (unsigned)output_config.enabled_mask);
  printf("current_temperature_compensation_c=%.3f\n", temperature.temperature_c);
  printf("current_probe_k=%.3f\n", probe_k.k_value);
  printf("current_calibration_level=%u\n", (unsigned)calibration.level);
  printf("vendor_guidance_dry_calibration_first=1\n");
  printf("vendor_guidance_keep_temperature_compensation_at_25c=1\n");
  printf("preview_samples=%u\n", (unsigned)preview_samples);
  printf("preview_interval_ms=%u\n", (unsigned)preview_interval_ms);
  printf("apply_requested=%d\n", apply_requested);
  if (step == STEP_SINGLE || step == STEP_LOW || step == STEP_HIGH) {
    printf("planned_reference_us=%.3f\n", reference_us);
  }
  if (probe_k_specified) {
    printf("planned_probe_k=%.3f\n", planned_probe_k);
  }

  if (step != STEP_STATUS && step != STEP_CLEAR) {
    result = preview_readings(&session.device,
                              output_config.enabled_mask,
                              preview_samples,
                              preview_interval_ms);
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("preview_readings", result);
    }
  }

  if (apply_requested) {
    if (probe_k_specified) {
      result = ezo_ec_send_probe_k_set_i2c(&session.device, planned_probe_k, 3, &hint);
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
      }
    }
    if (result == EZO_OK && step == STEP_CLEAR) {
      result = ezo_ec_send_clear_calibration_i2c(&session.device, &hint);
    } else if (result == EZO_OK && step != STEP_STATUS) {
      result = ezo_ec_send_calibration_i2c(&session.device,
                                           calibration_point_for_step(step),
                                           reference_us,
                                           0,
                                           &hint);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("send_calibration", result);
    }

    if (step != STEP_STATUS || probe_k_specified) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_send_probe_k_query_i2c(&session.device, &hint);
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_ec_read_probe_k_i2c(&session.device, &probe_k);
      }
      if (result == EZO_OK) {
        result = ezo_ec_send_calibration_query_i2c(&session.device, &hint);
      }
      if (result == EZO_OK) {
        ezo_example_wait_hint(&hint);
        result = ezo_ec_read_calibration_status_i2c(&session.device, &calibration);
      }
      if (result != EZO_OK) {
        ezo_example_close_i2c(&session);
        return ezo_example_print_error("post_calibration_query", result);
      }

      printf("post_probe_k=%.3f\n", probe_k.k_value);
      printf("post_calibration_level=%u\n", (unsigned)calibration.level);
    }
  }

  ezo_example_close_i2c(&session);
  return 0;
}
