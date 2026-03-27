/*
Purpose: inspect a full RTD + EC + pressure driven D.O. compensation chain and optionally apply all three inputs.
Defaults: /dev/i2c-1 with RTD at 102, EC at 100, D.O. at 97, and pressure 101.325 kPa.
Assumptions: all devices share the same I2C bus, the RTD and EC readings are representative, and EC salinity output is enabled before apply.
Next: read do_temp_comp_from_rtd.c and do_salinity_comp_from_ec.c for the narrower compensation-chain examples.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_products.h"

#include "ezo_do.h"
#include "ezo_ec.h"
#include "ezo_rtd.h"

#include <stdio.h>

int main(int argc, char **argv) {
  const char *device_path = EZO_EXAMPLE_I2C_DEFAULT_PATH;
  uint8_t rtd_address = 102U;
  uint8_t ec_address = 100U;
  uint8_t do_address = 97U;
  double pressure_kpa = 101.325;
  ezo_example_i2c_session_t rtd_session;
  ezo_example_i2c_session_t ec_session;
  ezo_example_i2c_session_t do_session;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t rtd_scale;
  ezo_rtd_reading_t rtd_reading;
  ezo_ec_output_config_t ec_output_config;
  ezo_ec_reading_t ec_reading;
  ezo_do_output_config_t do_output_config;
  ezo_do_temperature_compensation_t do_temperature;
  ezo_do_salinity_compensation_t do_salinity;
  ezo_do_pressure_compensation_t do_pressure;
  ezo_do_reading_t do_reading;
  double source_temperature_c = 0.0;
  double source_salinity_ppt = 0.0;
  int source_salinity_available = 0;
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
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--do-address=");
    if (value != NULL && !ezo_example_parse_uint8_arg(value, &do_address)) {
      fprintf(stderr, "invalid --do-address value\n");
      return 1;
    }
  }
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--pressure-kpa=");
    if (value != NULL && !ezo_example_parse_double_arg(value, &pressure_kpa)) {
      fprintf(stderr, "invalid --pressure-kpa value\n");
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
  result = ezo_example_open_i2c(device_path, do_address, &do_session);
  if (result != EZO_OK) {
    ezo_example_close_i2c(&ec_session);
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
    result = ezo_ec_send_output_query_i2c(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_output_config_i2c(&ec_session.device, &ec_output_config);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_read_i2c(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_i2c(&ec_session.device, ec_output_config.enabled_mask, &ec_reading);
  }
  if (result == EZO_OK) {
    source_salinity_available = (ec_reading.present_mask & EZO_EC_OUTPUT_SALINITY) != 0U;
    if (source_salinity_available) {
      source_salinity_ppt = ec_reading.salinity_ppt;
    }
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
    result = ezo_do_send_salinity_query_i2c(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_salinity_i2c(&do_session.device, &do_salinity);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_pressure_query_i2c(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_pressure_i2c(&do_session.device, &do_pressure);
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
    ezo_example_close_i2c(&ec_session);
    ezo_example_close_i2c(&rtd_session);
    return ezo_example_print_error("inspect_chain_state", result);
  }

  printf("transport=i2c\n");
  printf("bus_path=%s\n", device_path);
  printf("rtd_address=%u\n", (unsigned)rtd_address);
  printf("ec_address=%u\n", (unsigned)ec_address);
  printf("do_address=%u\n", (unsigned)do_address);
  printf("source_temperature_scale=%s\n", ezo_example_rtd_scale_name(rtd_reading.scale));
  printf("source_temperature=%.3f\n", rtd_reading.temperature);
  printf("source_temperature_c=%.3f\n", source_temperature_c);
  printf("source_ec_output_mask=%u\n", (unsigned)ec_output_config.enabled_mask);
  ezo_example_print_ec_reading("source_ec_", &ec_reading);
  printf("source_salinity_available=%d\n", source_salinity_available);
  if (source_salinity_available) {
    printf("source_salinity_ppt=%.3f\n", source_salinity_ppt);
  }
  printf("current_target_temperature_compensation_c=%.3f\n", do_temperature.temperature_c);
  printf("current_target_salinity_value=%.3f\n", do_salinity.value);
  printf("current_target_salinity_unit=%s\n", ezo_example_do_salinity_unit_name(do_salinity.unit));
  printf("current_target_pressure_kpa=%.3f\n", do_pressure.pressure_kpa);
  ezo_example_print_do_reading("current_target_", &do_reading);
  printf("vendor_note_ec_measurements_can_interfere=1\n");
  printf("apply_requested=%d\n", apply_requested);
  printf("planned_pressure_kpa=%.3f\n", pressure_kpa);
  if (!source_salinity_available) {
    printf("apply_blocked_reason=ec_salinity_output_disabled\n");
  } else {
    printf("planned_salinity_ppt=%.3f\n", source_salinity_ppt);
  }
  printf("planned_temperature_c=%.3f\n", source_temperature_c);

  if (apply_requested) {
    if (!source_salinity_available) {
      fprintf(stderr, "EC salinity output must be enabled before --apply\n");
      ezo_example_close_i2c(&do_session);
      ezo_example_close_i2c(&ec_session);
      ezo_example_close_i2c(&rtd_session);
      return 1;
    }

    result = ezo_do_send_temperature_set_i2c(&do_session.device, source_temperature_c, 3, &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_send_salinity_set_i2c(&do_session.device,
                                            source_salinity_ppt,
                                            EZO_DO_SALINITY_UNIT_PPT,
                                            3,
                                            &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_send_pressure_set_i2c(&do_session.device, pressure_kpa, 3, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_send_temperature_query_i2c(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_temperature_i2c(&do_session.device, &do_temperature);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_salinity_query_i2c(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_salinity_i2c(&do_session.device, &do_salinity);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_pressure_query_i2c(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_pressure_i2c(&do_session.device, &do_pressure);
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
      ezo_example_close_i2c(&ec_session);
      ezo_example_close_i2c(&rtd_session);
      return ezo_example_print_error("apply_full_compensation", result);
    }

    printf("post_target_temperature_compensation_c=%.3f\n", do_temperature.temperature_c);
    printf("post_target_salinity_value=%.3f\n", do_salinity.value);
    printf("post_target_salinity_unit=%s\n", ezo_example_do_salinity_unit_name(do_salinity.unit));
    printf("post_target_pressure_kpa=%.3f\n", do_pressure.pressure_kpa);
    ezo_example_print_do_reading("post_target_", &do_reading);
  }

  ezo_example_close_i2c(&do_session);
  ezo_example_close_i2c(&ec_session);
  ezo_example_close_i2c(&rtd_session);
  return 0;
}
