/*
Purpose: inspect pH compensation, calibration, slope, range, and one-shot RT,n behavior, with optional persistent setters.
Defaults: /dev/i2c-1 and the pH default address 99.
Assumptions: the connected device is a pH circuit in I2C mode.
Next: read ph_calibration.c for staged vendor-aligned calibration steps.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_ph.h"

#include <stdio.h>

int main(int argc, char **argv) {
  double planned_temperature_c = 25.0;
  double one_shot_temperature_c = 30.0;
  ezo_ph_extended_range_t planned_extended_range = EZO_PH_EXTENDED_RANGE_ENABLED;
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_ph_temperature_compensation_t temperature;
  ezo_ph_calibration_status_t calibration;
  ezo_ph_slope_t slope;
  ezo_ph_extended_range_status_t extended_range;
  ezo_ph_reading_t reading;
  ezo_ph_reading_t one_shot_reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_i2c_options(argc, argv, 99U, &options, &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [address] [--set-temperature-c=25.0] "
            "[--set-extended-range=0|1] [--rt-temperature-c=30.0] [--apply]\n",
            argv[0]);
    return 1;
  }

  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-temperature-c=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &planned_temperature_c)) {
    fprintf(stderr, "invalid --set-temperature-c value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--rt-temperature-c=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &one_shot_temperature_c)) {
    fprintf(stderr, "invalid --rt-temperature-c value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-extended-range=");
  if (value != NULL) {
    uint32_t enabled = 0;

    if (!ezo_example_parse_uint32_arg(value, &enabled) || enabled > 1U) {
      fprintf(stderr, "invalid --set-extended-range value\n");
      return 1;
    }

    planned_extended_range = enabled != 0U ? EZO_PH_EXTENDED_RANGE_ENABLED
                                           : EZO_PH_EXTENDED_RANGE_DISABLED;
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
  if (result == EZO_OK) {
    result = ezo_ph_send_extended_range_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_extended_range_i2c(&session.device, &extended_range);
  }
  if (result == EZO_OK) {
    result = ezo_ph_send_read_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_response_i2c(&session.device, &reading);
  }
  if (result == EZO_OK) {
    result = ezo_ph_send_read_with_temp_comp_i2c(&session.device,
                                                 one_shot_temperature_c,
                                                 2,
                                                 &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_response_i2c(&session.device, &one_shot_reading);
  }

  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_current_state", result);
  }

  printf("transport=i2c\n");
  printf("product=pH\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("current_temperature_compensation_c=%.3f\n", temperature.temperature_c);
  printf("current_calibration_level=%s\n", ezo_example_ph_calibration_name(calibration.level));
  printf("current_slope_acid_percent=%.3f\n", slope.acid_percent);
  printf("current_slope_base_percent=%.3f\n", slope.base_percent);
  printf("current_slope_neutral_mv=%.3f\n", slope.neutral_mv);
  printf("current_extended_range=%s\n",
         ezo_example_bool_name(extended_range.enabled == EZO_PH_EXTENDED_RANGE_ENABLED));
  printf("current_reading_ph=%.3f\n", reading.ph);
  printf("one_shot_rt_temperature_c=%.3f\n", one_shot_temperature_c);
  printf("one_shot_rt_reading_ph=%.3f\n", one_shot_reading.ph);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", planned_temperature_c);
  printf("planned_extended_range=%s\n",
         ezo_example_bool_name(planned_extended_range == EZO_PH_EXTENDED_RANGE_ENABLED));

  if (apply_requested) {
    result = ezo_ph_send_temperature_set_i2c(&session.device, planned_temperature_c, 2, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_send_extended_range_set_i2c(&session.device, planned_extended_range, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_send_temperature_query_i2c(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_read_temperature_i2c(&session.device, &temperature);
    }
    if (result == EZO_OK) {
      result = ezo_ph_send_extended_range_query_i2c(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_read_extended_range_i2c(&session.device, &extended_range);
    }
    if (result == EZO_OK) {
      result = ezo_ph_send_read_i2c(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_read_response_i2c(&session.device, &reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("apply_updates", result);
    }

    printf("post_temperature_compensation_c=%.3f\n", temperature.temperature_c);
    printf("post_extended_range=%s\n",
           ezo_example_bool_name(extended_range.enabled == EZO_PH_EXTENDED_RANGE_ENABLED));
    printf("post_reading_ph=%.3f\n", reading.ph);
  }

  ezo_example_close_i2c(&session);
  return 0;
}
