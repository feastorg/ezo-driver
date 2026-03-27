/*
Purpose: inspect HUM output selection, temperature-calibration state, and current reading, with optional output and Tcal setters.
Defaults: /dev/i2c-1 and the HUM default address 111.
Assumptions: the connected device is a HUM circuit in I2C mode.
Next: read hum_temperature_calibration.c for the staged temperature-calibration workflow.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_hum.h"

#include <stdio.h>

int main(int argc, char **argv) {
  double planned_reference_temp = 25.0;
  uint8_t enable_dew_point_output = 1U;
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_hum_output_config_t output_config;
  ezo_hum_temperature_calibration_status_t calibration;
  ezo_hum_reading_t reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_i2c_options(argc, argv, 111U, &options, &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [address] [--reference-temp=25.0] "
            "[--enable-dew-point=0|1] [--apply]\n",
            argv[0]);
    return 1;
  }
  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--reference-temp=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &planned_reference_temp)) {
    fprintf(stderr, "invalid --reference-temp value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--enable-dew-point=");
  if (value != NULL) {
    uint32_t enabled = 0;

    if (!ezo_example_parse_uint32_arg(value, &enabled) || enabled > 1U) {
      fprintf(stderr, "invalid --enable-dew-point value\n");
      return 1;
    }

    enable_dew_point_output = (uint8_t)enabled;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_hum_send_output_query_i2c(&session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_hum_read_output_config_i2c(&session.device, &output_config);
  }
  if (result == EZO_OK) {
    result = ezo_hum_send_temperature_calibration_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_hum_read_temperature_calibration_status_i2c(&session.device, &calibration);
  }
  if (result == EZO_OK) {
    result = ezo_hum_send_read_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_hum_read_response_i2c(&session.device, output_config.enabled_mask, &reading);
  }
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_current_state", result);
  }

  printf("transport=i2c\n");
  printf("product=HUM\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("current_output_mask=%u\n", (unsigned)output_config.enabled_mask);
  printf("current_temperature_calibrated=%u\n", (unsigned)calibration.calibrated);
  ezo_example_print_hum_reading("current_", &reading);
  printf("vendor_note_humidity_factory_calibrated=1\n");
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_reference_temp=%.3f\n", planned_reference_temp);
  printf("planned_dew_point_output=%s\n", ezo_example_bool_name(enable_dew_point_output != 0U));

  if (apply_requested) {
    result = ezo_hum_send_output_set_i2c(&session.device,
                                         EZO_HUM_OUTPUT_DEW_POINT,
                                         enable_dew_point_output,
                                         &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_hum_send_temperature_calibration_i2c(&session.device,
                                                        planned_reference_temp,
                                                        2,
                                                        &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_hum_send_output_query_i2c(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_hum_read_output_config_i2c(&session.device, &output_config);
    }
    if (result == EZO_OK) {
      result = ezo_hum_send_temperature_calibration_query_i2c(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_hum_read_temperature_calibration_status_i2c(&session.device, &calibration);
    }
    if (result == EZO_OK) {
      result = ezo_hum_send_read_i2c(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_hum_read_response_i2c(&session.device, output_config.enabled_mask, &reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("apply_updates", result);
    }

    printf("post_output_mask=%u\n", (unsigned)output_config.enabled_mask);
    printf("post_temperature_calibrated=%u\n", (unsigned)calibration.calibrated);
    ezo_example_print_hum_reading("post_", &reading);
  }

  ezo_example_close_i2c(&session);
  return 0;
}
