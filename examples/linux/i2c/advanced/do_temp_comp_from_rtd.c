/*
Purpose: inspect an RTD-driven D.O. temperature-compensation chain and optionally apply the source temperature.
Defaults: /dev/i2c-1 with RTD at 102 and D.O. at 97.
Assumptions: both devices share the same I2C bus and the RTD reading is valid for D.O. compensation.
Next: read do_workflow.c and do_full_compensation_chain.c for adjacent single-device and full-chain compensation flows.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_do.h"
#include "ezo_rtd.h"

#include <stdio.h>

int main(int argc, char **argv) {
  const char *device_path = EZO_EXAMPLE_I2C_DEFAULT_PATH;
  uint8_t rtd_address = 102U;
  uint8_t do_address = 97U;
  ezo_example_i2c_session_t rtd_session;
  ezo_example_i2c_session_t do_session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_do_output_config_t do_output_config;
  ezo_do_temperature_compensation_t do_temperature;
  ezo_do_reading_t do_reading;
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
    const char *value = ezo_example_find_option_value(argc, argv, index, "--do-address=");
    if (value != NULL && !ezo_example_parse_uint8_arg(value, &do_address)) {
      fprintf(stderr, "invalid --do-address value\n");
      return 1;
    }
  }

  result = ezo_example_open_i2c(device_path, rtd_address, &rtd_session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_rtd_i2c", result);
  }

  result = ezo_example_open_i2c(device_path, do_address, &do_session);
  if (result != EZO_OK) {
    ezo_example_close_i2c(&rtd_session);
    return ezo_example_print_error("open_do_i2c", result);
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
  if (result == EZO_OK &&
      !ezo_example_rtd_reading_to_celsius(&rtd_reading, &source_temperature_c)) {
    result = EZO_ERR_PROTOCOL;
  }
  if (result == EZO_OK) {
    result = ezo_do_send_output_query_i2c(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_output_config_i2c(&do_session.device, &do_output_config);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_temperature_query_i2c(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_temperature_i2c(&do_session.device, &do_temperature);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_read_i2c(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_response_i2c(&do_session.device, do_output_config.enabled_mask, &do_reading);
  }

  if (result != EZO_OK) {
    ezo_example_close_i2c(&do_session);
    ezo_example_close_i2c(&rtd_session);
    return ezo_example_print_error("inspect_chain_state", result);
  }

  printf("transport=i2c\n");
  printf("bus_path=%s\n", device_path);
  printf("rtd_address=%u\n", (unsigned)rtd_address);
  printf("do_address=%u\n", (unsigned)do_address);
  printf("source_scale=%s\n", ezo_example_rtd_scale_name(rtd_reading.scale));
  printf("source_temperature=%.3f\n", rtd_reading.temperature);
  printf("source_temperature_c=%.3f\n", source_temperature_c);
  printf("target_output_mask=%u\n", (unsigned)do_output_config.enabled_mask);
  printf("current_target_temperature_compensation_c=%.3f\n", do_temperature.temperature_c);
  ezo_example_print_do_reading("current_target_", &do_reading);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", source_temperature_c);

  if (apply_requested) {
    result = ezo_do_send_temperature_set_i2c(&do_session.device, source_temperature_c, 3, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_send_temperature_query_i2c(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_temperature_i2c(&do_session.device, &do_temperature);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_read_i2c(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_response_i2c(&do_session.device, do_output_config.enabled_mask, &do_reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&do_session);
      ezo_example_close_i2c(&rtd_session);
      return ezo_example_print_error("apply_temperature_compensation", result);
    }

    printf("post_target_temperature_compensation_c=%.3f\n", do_temperature.temperature_c);
    ezo_example_print_do_reading("post_target_", &do_reading);
  }

  ezo_example_close_i2c(&do_session);
  ezo_example_close_i2c(&rtd_session);
  return 0;
}
