/*
Purpose: inspect an RTD-driven pH temperature-compensation chain and optionally apply the source temperature.
Defaults: /dev/i2c-1 with RTD at 102 and pH at 99.
Assumptions: both devices share the same I2C bus and the RTD reading is valid for pH compensation.
Next: read ph_workflow.c and ec_temp_comp_from_rtd.c for adjacent single-device and cross-device compensation flows.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_ph.h"
#include "ezo_rtd.h"

#include <stdio.h>

int main(int argc, char **argv) {
  const char *device_path = EZO_EXAMPLE_I2C_DEFAULT_PATH;
  uint8_t rtd_address = 102U;
  uint8_t ph_address = 99U;
  ezo_example_i2c_session_t rtd_session;
  ezo_example_i2c_session_t ph_session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_ph_temperature_compensation_t ph_temperature;
  ezo_ph_reading_t ph_reading;
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
    const char *value = ezo_example_find_option_value(argc, argv, index, "--ph-address=");
    if (value != NULL && !ezo_example_parse_uint8_arg(value, &ph_address)) {
      fprintf(stderr, "invalid --ph-address value\n");
      return 1;
    }
  }

  result = ezo_example_open_i2c(device_path, rtd_address, &rtd_session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_rtd_i2c", result);
  }

  result = ezo_example_open_i2c(device_path, ph_address, &ph_session);
  if (result != EZO_OK) {
    ezo_example_close_i2c(&rtd_session);
    return ezo_example_print_error("open_ph_i2c", result);
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
    result = ezo_ph_send_temperature_query_i2c(&ph_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_temperature_i2c(&ph_session.device, &ph_temperature);
  }
  if (result == EZO_OK) {
    result = ezo_ph_send_read_i2c(&ph_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ph_read_response_i2c(&ph_session.device, &ph_reading);
  }

  if (result != EZO_OK) {
    ezo_example_close_i2c(&ph_session);
    ezo_example_close_i2c(&rtd_session);
    return ezo_example_print_error("inspect_chain_state", result);
  }

  printf("transport=i2c\n");
  printf("bus_path=%s\n", device_path);
  printf("rtd_address=%u\n", (unsigned)rtd_address);
  printf("ph_address=%u\n", (unsigned)ph_address);
  printf("source_scale=%s\n", ezo_example_rtd_scale_name(rtd_reading.scale));
  printf("source_temperature=%.3f\n", rtd_reading.temperature);
  printf("source_temperature_c=%.3f\n", source_temperature_c);
  printf("current_target_temperature_compensation_c=%.3f\n", ph_temperature.temperature_c);
  printf("current_target_reading_ph=%.3f\n", ph_reading.ph);
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_temperature_c=%.3f\n", source_temperature_c);

  if (apply_requested) {
    result = ezo_ph_send_temperature_set_i2c(&ph_session.device, source_temperature_c, 3, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_send_temperature_query_i2c(&ph_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_read_temperature_i2c(&ph_session.device, &ph_temperature);
    }
    if (result == EZO_OK) {
      result = ezo_ph_send_read_i2c(&ph_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_ph_read_response_i2c(&ph_session.device, &ph_reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_i2c(&ph_session);
      ezo_example_close_i2c(&rtd_session);
      return ezo_example_print_error("apply_temperature_compensation", result);
    }

    printf("post_target_temperature_compensation_c=%.3f\n", ph_temperature.temperature_c);
    printf("post_target_reading_ph=%.3f\n", ph_reading.ph);
  }

  ezo_example_close_i2c(&ph_session);
  ezo_example_close_i2c(&rtd_session);
  return 0;
}
