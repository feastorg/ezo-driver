/*
Purpose: inspect an RTD-driven EC temperature-compensation chain and optionally apply the source temperature.
Defaults: /dev/i2c-1 with RTD at 102 and EC at 100.
Assumptions: both devices share the same I2C bus and the RTD reading is valid for EC compensation.
Next: read rtd_workflow.c and do_salinity_comp_from_ec.c for the adjacent cross-device stateful flows.
*/

#include "example_base.h"
#include "example_i2c.h"

#include "ezo_ec.h"
#include "ezo_rtd.h"

#include <stdio.h>

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

static int reading_to_celsius(const ezo_rtd_reading_t *reading, double *temperature_c_out) {
  if (reading == NULL || temperature_c_out == NULL) {
    return 0;
  }

  switch (reading->scale) {
    case EZO_RTD_SCALE_CELSIUS:
      *temperature_c_out = reading->temperature;
      return 1;
    case EZO_RTD_SCALE_KELVIN:
      *temperature_c_out = reading->temperature - 273.15;
      return 1;
    case EZO_RTD_SCALE_FAHRENHEIT:
      *temperature_c_out = (reading->temperature - 32.0) * (5.0 / 9.0);
      return 1;
    case EZO_RTD_SCALE_UNKNOWN:
    default:
      return 0;
  }
}

static void print_ec_reading(const char *prefix, const ezo_ec_reading_t *reading) {
  if ((reading->present_mask & EZO_EC_OUTPUT_CONDUCTIVITY) != 0U) {
    printf("%sconductivity_us_cm=%.3f\n", prefix, reading->conductivity_us_cm);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS) != 0U) {
    printf("%stotal_dissolved_solids_ppm=%.3f\n", prefix, reading->total_dissolved_solids_ppm);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SALINITY) != 0U) {
    printf("%ssalinity_ppt=%.3f\n", prefix, reading->salinity_ppt);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SPECIFIC_GRAVITY) != 0U) {
    printf("%sspecific_gravity=%.3f\n", prefix, reading->specific_gravity);
  }
}

int main(int argc, char **argv) {
  const char *device_path = EZO_EXAMPLE_I2C_DEFAULT_PATH;
  uint8_t rtd_address = 102U;
  uint8_t ec_address = 100U;
  ezo_example_i2c_session_t rtd_session;
  ezo_example_i2c_session_t ec_session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_ec_output_config_t ec_output_config;
  ezo_ec_temperature_compensation_t ec_temperature;
  ezo_ec_reading_t ec_reading;
  double source_temperature_c = 0.0;
  ezo_result_t result = EZO_OK;
  int index = 1;
  int apply_requested = 0;

  if (argc > index && argv[index][0] != '-') {
    device_path = argv[index];
    index += 1;
  }

  apply_requested = ezo_example_has_flag(argc, argv, index, "--apply");

  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--rtd-address=");
    if (value != NULL && !ezo_example_parse_uint8_arg(value, &rtd_address)) {
      fprintf(stderr, "invalid --rtd-address value\n");
      return 1;
    }
  }
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--ec-address=");
    if (value != NULL && !ezo_example_parse_uint8_arg(value, &ec_address)) {
      fprintf(stderr, "invalid --ec-address value\n");
      return 1;
    }
  }

  result = ezo_example_open_i2c(device_path, rtd_address, &rtd_session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_rtd_i2c", result);
  }

  result = ezo_example_open_i2c(device_path, ec_address, &ec_session);
  if (result != EZO_OK) {
    ezo_example_close_i2c(&rtd_session);
    return ezo_example_print_error("open_ec_i2c", result);
  }

  result = ezo_rtd_send_scale_query_i2c(&rtd_session.device, &hint);
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_scale_i2c(&rtd_session.device, &rtd_scale);
  }
  if (result == EZO_OK) {
    result = ezo_rtd_send_read_i2c(&rtd_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_rtd_read_response_i2c(&rtd_session.device, rtd_scale.scale, &rtd_reading);
  }
  if (result == EZO_OK && !reading_to_celsius(&rtd_reading, &source_temperature_c)) {
    result = EZO_ERR_PROTOCOL;
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_output_query_i2c(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_output_config_i2c(&ec_session.device, &ec_output_config);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_temperature_query_i2c(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_temperature_i2c(&ec_session.device, &ec_temperature);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_read_i2c(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_i2c(&ec_session.device, ec_output_config.enabled_mask, &ec_reading);
  }

  if (result != EZO_OK) {
    ezo_example_close_i2c(&ec_session);
    ezo_example_close_i2c(&rtd_session);
    return ezo_example_print_error("inspect_chain_state", result);
  }

  printf("transport=i2c\n");
  printf("bus_path=%s\n", device_path);
  printf("rtd_address=%u\n", (unsigned)rtd_address);
  printf("ec_address=%u\n", (unsigned)ec_address);
  printf("source_scale=%s\n", scale_name(rtd_reading.scale));
  printf("source_temperature=%.3f\n", rtd_reading.temperature);
  printf("source_temperature_c=%.3f\n", source_temperature_c);
  printf("target_output_mask=%u\n", (unsigned)ec_output_config.enabled_mask);
  printf("current_target_temperature_compensation_c=%.3f\n", ec_temperature.temperature_c);
  print_ec_reading("current_target_", &ec_reading);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", source_temperature_c);

  if (apply_requested) {
    result = ezo_ec_send_temperature_set_i2c(&ec_session.device, source_temperature_c, 3, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_send_temperature_query_i2c(&ec_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_read_temperature_i2c(&ec_session.device, &ec_temperature);
    }
    if (result == EZO_OK) {
      result = ezo_ec_send_read_i2c(&ec_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ec_read_response_i2c(&ec_session.device, ec_output_config.enabled_mask, &ec_reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&ec_session);
      ezo_example_close_i2c(&rtd_session);
      return ezo_example_print_error("apply_temperature_compensation", result);
    }

    printf("post_target_temperature_compensation_c=%.3f\n", ec_temperature.temperature_c);
    print_ec_reading("post_target_", &ec_reading);
  }

  ezo_example_close_i2c(&ec_session);
  ezo_example_close_i2c(&rtd_session);
  return 0;
}
