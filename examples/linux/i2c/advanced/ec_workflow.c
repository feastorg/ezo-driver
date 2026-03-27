/*
Purpose: inspect EC output selection, temperature compensation, probe-K, TDS factor, calibration, and one-shot RT,n behavior.
Defaults: /dev/i2c-1 and the EC default address 100.
Assumptions: the connected device is an EC circuit in I2C mode.
Next: read ec_calibration.c for staged dry, single, low, and high calibration steps.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_ec.h"

#include <stdio.h>

int main(int argc, char **argv) {
  double planned_temperature_c = 25.0;
  double planned_probe_k = 1.0;
  double planned_tds_factor = 0.5;
  double one_shot_temperature_c = 30.0;
  uint8_t enable_salinity_output = 1U;
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_timing_hint_t hint;
  ezo_ec_output_config_t output_config;
  ezo_ec_temperature_compensation_t temperature;
  ezo_ec_probe_k_t probe_k;
  ezo_ec_tds_factor_t tds_factor;
  ezo_ec_calibration_status_t calibration;
  ezo_ec_reading_t reading;
  ezo_ec_reading_t one_shot_reading;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;
  int apply_requested = 0;
  const char *value = NULL;

  if (!ezo_example_parse_i2c_options(argc, argv, 100U, &options, &next_arg)) {
    fprintf(stderr,
            "usage: %s [device_path] [address] [--set-temperature-c=25.0] "
            "[--set-probe-k=1.0] [--set-tds-factor=0.5] [--enable-salinity-output=0|1] "
            "[--rt-temperature-c=30.0] [--apply]\n",
            argv[0]);
    return 1;
  }
  apply_requested = ezo_example_has_flag(argc, argv, next_arg, "--apply");

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-temperature-c=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &planned_temperature_c)) {
    fprintf(stderr, "invalid --set-temperature-c value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-probe-k=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &planned_probe_k)) {
    fprintf(stderr, "invalid --set-probe-k value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--set-tds-factor=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &planned_tds_factor)) {
    fprintf(stderr, "invalid --set-tds-factor value\n");
    return 1;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--enable-salinity-output=");
  if (value != NULL) {
    uint32_t enabled = 0;

    if (!ezo_example_parse_uint32_arg(value, &enabled) || enabled > 1U) {
      fprintf(stderr, "invalid --enable-salinity-output value\n");
      return 1;
    }

    enable_salinity_output = (uint8_t)enabled;
  }

  value = ezo_example_find_option_value(argc, argv, next_arg, "--rt-temperature-c=");
  if (value != NULL && !ezo_example_parse_double_arg(value, &one_shot_temperature_c)) {
    fprintf(stderr, "invalid --rt-temperature-c value\n");
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
    result = ezo_ec_send_tds_factor_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_tds_factor_i2c(&session.device, &tds_factor);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_calibration_query_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_calibration_status_i2c(&session.device, &calibration);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_read_i2c(&session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_i2c(&session.device, output_config.enabled_mask, &reading);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_read_with_temp_comp_i2c(&session.device,
                                                 one_shot_temperature_c,
                                                 2,
                                                 &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_i2c(&session.device,
                                      output_config.enabled_mask,
                                      &one_shot_reading);
  }
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_current_state", result);
  }

  printf("transport=i2c\n");
  printf("product=EC\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("current_output_mask=%u\n", (unsigned)output_config.enabled_mask);
  printf("current_temperature_compensation_c=%.3f\n", temperature.temperature_c);
  printf("current_probe_k=%.3f\n", probe_k.k_value);
  printf("current_tds_factor=%.3f\n", tds_factor.factor);
  printf("current_calibration_level=%u\n", (unsigned)calibration.level);
  ezo_example_print_ec_reading("current_", &reading);
  printf("one_shot_rt_temperature_c=%.3f\n", one_shot_temperature_c);
  ezo_example_print_ec_reading("one_shot_rt_", &one_shot_reading);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", planned_temperature_c);
  printf("planned_probe_k=%.3f\n", planned_probe_k);
  printf("planned_tds_factor=%.3f\n", planned_tds_factor);
  printf("planned_salinity_output=%s\n", ezo_example_bool_name(enable_salinity_output != 0U));

  if (apply_requested) {
    result = ezo_ec_send_temperature_set_i2c(&session.device, planned_temperature_c, 2, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_send_probe_k_set_i2c(&session.device, planned_probe_k, 3, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_send_tds_factor_set_i2c(&session.device, planned_tds_factor, 3, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_send_output_set_i2c(&session.device,
                                          EZO_EC_OUTPUT_SALINITY,
                                          enable_salinity_output,
                                          &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_send_output_query_i2c(&session.device, &hint);
    }
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
      result = ezo_ec_send_tds_factor_query_i2c(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_read_tds_factor_i2c(&session.device, &tds_factor);
    }
    if (result == EZO_OK) {
      result = ezo_ec_send_read_i2c(&session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_read_response_i2c(&session.device, output_config.enabled_mask, &reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&session);
      return ezo_example_print_error("apply_updates", result);
    }

    printf("post_output_mask=%u\n", (unsigned)output_config.enabled_mask);
    printf("post_temperature_compensation_c=%.3f\n", temperature.temperature_c);
    printf("post_probe_k=%.3f\n", probe_k.k_value);
    printf("post_tds_factor=%.3f\n", tds_factor.factor);
    ezo_example_print_ec_reading("post_", &reading);
  }

  ezo_example_close_i2c(&session);
  return 0;
}
