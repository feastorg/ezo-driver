#include "example_base.h"
#include "example_product_uart.h"

#include "ezo_do.h"
#include "ezo_ec.h"
#include "ezo_hum.h"
#include "ezo_orp.h"
#include "ezo_ph.h"
#include "ezo_rtd.h"

#include <stdio.h>

#define EZO_EXAMPLE_UART_QUERY(send_expr, read_expr) \
  do {                                               \
    result = (send_expr);                            \
    if (result != EZO_OK) {                          \
      return result;                                 \
    }                                                \
    ezo_example_wait_hint(&hint);                    \
    result = (read_expr);                            \
    if (result != EZO_OK) {                          \
      return result;                                 \
    }                                                \
  } while (0)

static const char *ezo_example_ph_calibration_name(
    ezo_ph_calibration_level_t level) {
  switch (level) {
    case EZO_PH_CALIBRATION_NONE:
      return "none";
    case EZO_PH_CALIBRATION_ONE_POINT:
      return "one_point";
    case EZO_PH_CALIBRATION_TWO_POINT:
      return "two_point";
    case EZO_PH_CALIBRATION_THREE_POINT:
      return "three_point";
    default:
      return "unknown";
  }
}

static const char *ezo_example_orp_extended_scale_name(
    ezo_orp_extended_scale_t extended_scale) {
  return extended_scale == EZO_ORP_EXTENDED_SCALE_ENABLED ? "enabled" : "disabled";
}

static const char *ezo_example_rtd_scale_name(ezo_rtd_scale_t scale) {
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

static const char *ezo_example_do_salinity_unit_name(
    ezo_do_salinity_unit_t unit) {
  return unit == EZO_DO_SALINITY_UNIT_PPT ? "ppt" : "us_cm";
}

ezo_result_t ezo_example_query_info_uart(ezo_uart_device_t *device,
                                         ezo_device_info_t *info_out) {
  ezo_timing_hint_t hint;
  ezo_result_t result = EZO_OK;

  if (device == NULL || info_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_control_send_info_query_uart(device, EZO_PRODUCT_UNKNOWN, &hint);
  if (result != EZO_OK) {
    return result;
  }

  ezo_example_wait_hint(&hint);
  return ezo_control_read_info_uart(device, info_out);
}

ezo_result_t ezo_example_print_shared_control_uart(ezo_uart_device_t *device,
                                                   ezo_product_id_t product_id) {
  ezo_timing_hint_t hint;
  ezo_control_name_t name;
  ezo_control_status_t status;
  ezo_control_led_status_t led;
  ezo_control_protocol_lock_status_t protocol_lock;
  ezo_control_baud_status_t baud;
  ezo_control_response_code_status_t response_code;
  ezo_result_t result = EZO_OK;

  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  EZO_EXAMPLE_UART_QUERY(ezo_control_send_name_query_uart(device, product_id, &hint),
                         ezo_control_read_name_uart(device, &name));
  EZO_EXAMPLE_UART_QUERY(ezo_control_send_status_query_uart(device, product_id, &hint),
                         ezo_control_read_status_uart(device, &status));
  EZO_EXAMPLE_UART_QUERY(ezo_control_send_led_query_uart(device, product_id, &hint),
                         ezo_control_read_led_uart(device, &led));
  EZO_EXAMPLE_UART_QUERY(
      ezo_control_send_protocol_lock_query_uart(device, product_id, &hint),
      ezo_control_read_protocol_lock_uart(device, &protocol_lock));
  EZO_EXAMPLE_UART_QUERY(ezo_control_send_baud_query_uart(device, product_id, &hint),
                         ezo_control_read_baud_uart(device, &baud));
  EZO_EXAMPLE_UART_QUERY(
      ezo_control_send_response_code_query_uart(device, product_id, &hint),
      ezo_control_read_response_code_uart(device, &response_code));

  printf("device_name=%s\n", name.name);
  printf("restart_code=%c\n", status.restart_code);
  printf("supply_voltage_v=%.3f\n", status.supply_voltage);
  printf("led_enabled=%u\n", (unsigned)led.enabled);
  printf("protocol_lock_enabled=%u\n", (unsigned)protocol_lock.enabled);
  printf("baud_rate=%u\n", (unsigned)baud.baud_rate);
  printf("response_codes_enabled=%u\n", (unsigned)response_code.enabled);
  return EZO_OK;
}

ezo_result_t ezo_example_print_product_readiness_uart(ezo_uart_device_t *device,
                                                      ezo_product_id_t product_id) {
  ezo_timing_hint_t hint;
  ezo_result_t result = EZO_OK;

  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  switch (product_id) {
    case EZO_PRODUCT_PH: {
      ezo_ph_temperature_compensation_t temperature;
      ezo_ph_calibration_status_t calibration;
      ezo_ph_slope_t slope;
      ezo_ph_extended_range_status_t extended_range;

      EZO_EXAMPLE_UART_QUERY(ezo_ph_send_temperature_query_uart(device, &hint),
                             ezo_ph_read_temperature_uart(device, &temperature));
      EZO_EXAMPLE_UART_QUERY(ezo_ph_send_calibration_query_uart(device, &hint),
                             ezo_ph_read_calibration_status_uart(device, &calibration));
      EZO_EXAMPLE_UART_QUERY(ezo_ph_send_slope_query_uart(device, &hint),
                             ezo_ph_read_slope_uart(device, &slope));
      EZO_EXAMPLE_UART_QUERY(ezo_ph_send_extended_range_query_uart(device, &hint),
                             ezo_ph_read_extended_range_uart(device, &extended_range));

      printf("ph_temperature_compensation_c=%.3f\n", temperature.temperature_c);
      printf("ph_calibration_level=%s\n",
             ezo_example_ph_calibration_name(calibration.level));
      printf("ph_slope_acid_percent=%.3f\n", slope.acid_percent);
      printf("ph_slope_base_percent=%.3f\n", slope.base_percent);
      printf("ph_slope_neutral_mv=%.3f\n", slope.neutral_mv);
      printf("ph_extended_range=%s\n",
             ezo_example_bool_name(extended_range.enabled ==
                                   EZO_PH_EXTENDED_RANGE_ENABLED));
      printf("readiness_supported=1\n");
      return EZO_OK;
    }
    case EZO_PRODUCT_ORP: {
      ezo_orp_calibration_status_t calibration;
      ezo_orp_extended_scale_status_t extended_scale;

      EZO_EXAMPLE_UART_QUERY(ezo_orp_send_calibration_query_uart(device, &hint),
                             ezo_orp_read_calibration_status_uart(device, &calibration));
      EZO_EXAMPLE_UART_QUERY(ezo_orp_send_extended_scale_query_uart(device, &hint),
                             ezo_orp_read_extended_scale_uart(device, &extended_scale));

      printf("orp_calibrated=%u\n", (unsigned)calibration.calibrated);
      printf("orp_extended_scale=%s\n",
             ezo_example_orp_extended_scale_name(extended_scale.enabled));
      printf("readiness_supported=1\n");
      return EZO_OK;
    }
    case EZO_PRODUCT_EC: {
      ezo_ec_output_config_t output_config;
      ezo_ec_temperature_compensation_t temperature;
      ezo_ec_probe_k_t probe_k;
      ezo_ec_tds_factor_t tds_factor;
      ezo_ec_calibration_status_t calibration;

      EZO_EXAMPLE_UART_QUERY(ezo_ec_send_output_query_uart(device, &hint),
                             ezo_ec_read_output_config_uart(device, &output_config));
      EZO_EXAMPLE_UART_QUERY(ezo_ec_send_temperature_query_uart(device, &hint),
                             ezo_ec_read_temperature_uart(device, &temperature));
      EZO_EXAMPLE_UART_QUERY(ezo_ec_send_probe_k_query_uart(device, &hint),
                             ezo_ec_read_probe_k_uart(device, &probe_k));
      EZO_EXAMPLE_UART_QUERY(ezo_ec_send_tds_factor_query_uart(device, &hint),
                             ezo_ec_read_tds_factor_uart(device, &tds_factor));
      EZO_EXAMPLE_UART_QUERY(ezo_ec_send_calibration_query_uart(device, &hint),
                             ezo_ec_read_calibration_status_uart(device, &calibration));

      printf("ec_output_mask=%u\n", (unsigned)output_config.enabled_mask);
      printf("ec_temperature_compensation_c=%.3f\n", temperature.temperature_c);
      printf("ec_probe_k=%.3f\n", probe_k.k_value);
      printf("ec_tds_factor=%.3f\n", tds_factor.factor);
      printf("ec_calibration_level=%u\n", (unsigned)calibration.level);
      printf("readiness_supported=1\n");
      return EZO_OK;
    }
    case EZO_PRODUCT_DO: {
      ezo_do_output_config_t output_config;
      ezo_do_temperature_compensation_t temperature;
      ezo_do_salinity_compensation_t salinity;
      ezo_do_pressure_compensation_t pressure;
      ezo_do_calibration_status_t calibration;

      EZO_EXAMPLE_UART_QUERY(ezo_do_send_output_query_uart(device, &hint),
                             ezo_do_read_output_config_uart(device, &output_config));
      EZO_EXAMPLE_UART_QUERY(ezo_do_send_temperature_query_uart(device, &hint),
                             ezo_do_read_temperature_uart(device, &temperature));
      EZO_EXAMPLE_UART_QUERY(ezo_do_send_salinity_query_uart(device, &hint),
                             ezo_do_read_salinity_uart(device, &salinity));
      EZO_EXAMPLE_UART_QUERY(ezo_do_send_pressure_query_uart(device, &hint),
                             ezo_do_read_pressure_uart(device, &pressure));
      EZO_EXAMPLE_UART_QUERY(ezo_do_send_calibration_query_uart(device, &hint),
                             ezo_do_read_calibration_status_uart(device, &calibration));

      printf("do_output_mask=%u\n", (unsigned)output_config.enabled_mask);
      printf("do_temperature_compensation_c=%.3f\n", temperature.temperature_c);
      printf("do_salinity_value=%.3f\n", salinity.value);
      printf("do_salinity_unit=%s\n", ezo_example_do_salinity_unit_name(salinity.unit));
      printf("do_pressure_kpa=%.3f\n", pressure.pressure_kpa);
      printf("do_calibration_level=%u\n", (unsigned)calibration.level);
      printf("readiness_supported=1\n");
      return EZO_OK;
    }
    case EZO_PRODUCT_RTD: {
      ezo_rtd_scale_status_t scale;
      ezo_rtd_calibration_status_t calibration;
      ezo_rtd_logger_status_t logger;
      ezo_rtd_memory_status_t memory;

      EZO_EXAMPLE_UART_QUERY(ezo_rtd_send_scale_query_uart(device, &hint),
                             ezo_rtd_read_scale_uart(device, &scale));
      EZO_EXAMPLE_UART_QUERY(ezo_rtd_send_calibration_query_uart(device, &hint),
                             ezo_rtd_read_calibration_status_uart(device, &calibration));
      EZO_EXAMPLE_UART_QUERY(ezo_rtd_send_logger_query_uart(device, &hint),
                             ezo_rtd_read_logger_uart(device, &logger));
      EZO_EXAMPLE_UART_QUERY(ezo_rtd_send_memory_query_uart(device, &hint),
                             ezo_rtd_read_memory_status_uart(device, &memory));

      printf("rtd_scale=%s\n", ezo_example_rtd_scale_name(scale.scale));
      printf("rtd_calibrated=%u\n", (unsigned)calibration.calibrated);
      printf("rtd_logger_interval_units=%u\n", (unsigned)logger.interval_units);
      printf("rtd_memory_last_index=%u\n", (unsigned)memory.last_index);
      printf("readiness_supported=1\n");
      return EZO_OK;
    }
    case EZO_PRODUCT_HUM: {
      ezo_hum_output_config_t output_config;
      ezo_hum_temperature_calibration_status_t calibration;

      EZO_EXAMPLE_UART_QUERY(ezo_hum_send_output_query_uart(device, &hint),
                             ezo_hum_read_output_config_uart(device, &output_config));
      EZO_EXAMPLE_UART_QUERY(
          ezo_hum_send_temperature_calibration_query_uart(device, &hint),
          ezo_hum_read_temperature_calibration_status_uart(device, &calibration));

      printf("hum_output_mask=%u\n", (unsigned)output_config.enabled_mask);
      printf("hum_temperature_calibrated=%u\n", (unsigned)calibration.calibrated);
      printf("readiness_supported=1\n");
      return EZO_OK;
    }
    default:
      printf("readiness_supported=0\n");
      return EZO_OK;
  }
}
