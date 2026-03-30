/*
Purpose: inspect an I2C device and print product-specific readiness signals.
Defaults: /dev/i2c-1 and address 99.
Assumptions: the device is already in I2C mode and the address is known.
Next: jump to ../typed/read_<product>.c for a simple read or ../advanced/ for stateful flows.
*/

#include "example_base.h"
#include "example_control.h"
#include "example_i2c.h"

#include "ezo_do.h"
#include "ezo_ec.h"
#include "ezo_hum.h"
#include "ezo_orp.h"
#include "ezo_ph.h"
#include "ezo_product.h"
#include "ezo_rtd.h"

#include <stdio.h>

#define I2C_QUERY(send_expr, read_expr) \
  do {                                  \
    result = (send_expr);               \
    if (result != EZO_OK) {             \
      return result;                    \
    }                                   \
    ezo_example_wait_hint(&hint);       \
    result = (read_expr);               \
    if (result != EZO_OK) {             \
      return result;                    \
    }                                   \
  } while (0)

static const char *support_name(ezo_product_support_t support) {
  switch (support) {
    case EZO_PRODUCT_SUPPORT_METADATA:
      return "metadata";
    case EZO_PRODUCT_SUPPORT_TYPED_READ:
      return "typed_read";
    case EZO_PRODUCT_SUPPORT_FULL:
      return "full";
    default:
      return "unknown";
  }
}

static const char *transport_name(ezo_product_transport_t transport) {
  switch (transport) {
    case EZO_PRODUCT_TRANSPORT_UART:
      return "uart";
    case EZO_PRODUCT_TRANSPORT_I2C:
      return "i2c";
    default:
      return "unknown";
  }
}

static const char *default_state_name(ezo_product_default_state_t state) {
  switch (state) {
    case EZO_PRODUCT_DEFAULT_DISABLED:
      return "disabled";
    case EZO_PRODUCT_DEFAULT_ENABLED:
      return "enabled";
    case EZO_PRODUCT_DEFAULT_QUERY_REQUIRED:
      return "query_required";
    default:
      return "unknown";
  }
}

static const char *output_schema_name(ezo_product_output_schema_t schema) {
  switch (schema) {
    case EZO_PRODUCT_OUTPUT_SCHEMA_SCALAR_SINGLE:
      return "scalar_single";
    case EZO_PRODUCT_OUTPUT_SCHEMA_PRIMARY_ONLY:
      return "primary_only";
    case EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED:
      return "query_required";
    default:
      return "unknown";
  }
}

static const char *bool_name(int value) {
  return value ? "enabled" : "disabled";
}

static const char *ph_calibration_name(ezo_ph_calibration_level_t level) {
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

static const char *orp_extended_scale_name(ezo_orp_extended_scale_t extended_scale) {
  return extended_scale == EZO_ORP_EXTENDED_SCALE_ENABLED ? "enabled" : "disabled";
}

static const char *rtd_scale_name(ezo_rtd_scale_t scale) {
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

static const char *do_salinity_unit_name(ezo_do_salinity_unit_t unit) {
  return unit == EZO_DO_SALINITY_UNIT_PPT ? "ppt" : "us_cm";
}

static void print_device_info(const ezo_device_info_t *info) {
  if (info == NULL) {
    return;
  }

  printf("product_id=%d\n", (int)info->product_id);
  printf("product_code=%s\n", info->product_code);
  printf("firmware_version=%s\n", info->firmware_version);
}

static void print_product_metadata(const ezo_product_metadata_t *metadata) {
  if (metadata == NULL) {
    printf("metadata_known=0\n");
    return;
  }

  printf("metadata_known=1\n");
  printf("family_name=%s\n", metadata->family_name);
  printf("vendor_short_code=%s\n", metadata->vendor_short_code);
  printf("support_tier=%s\n", support_name(metadata->support_tier));
  printf("default_transport=%s\n", transport_name(metadata->default_transport));
  printf("default_i2c_address=%u\n", (unsigned)metadata->default_i2c_address);
  printf("default_continuous_mode=%s\n",
         default_state_name(metadata->default_continuous_mode));
  printf("default_response_codes=%s\n",
         default_state_name(metadata->default_response_codes));
  printf("default_output_schema=%s\n",
         output_schema_name(metadata->default_output_schema));
  printf("default_output_count=%u\n", (unsigned)metadata->default_output_count);
  printf("capability_flags=%u\n", (unsigned)metadata->capability_flags);
  printf("command_family_flags=%u\n", (unsigned)metadata->command_family_flags);
  printf("uart_timing_generic_ms=%u\n", (unsigned)metadata->uart_timing.generic_ms);
  printf("uart_timing_read_ms=%u\n", (unsigned)metadata->uart_timing.read_ms);
  printf("uart_timing_read_with_temp_comp_ms=%u\n",
         (unsigned)metadata->uart_timing.read_with_temp_comp_ms);
  printf("uart_timing_calibration_ms=%u\n", (unsigned)metadata->uart_timing.calibration_ms);
  printf("i2c_timing_generic_ms=%u\n", (unsigned)metadata->i2c_timing.generic_ms);
  printf("i2c_timing_read_ms=%u\n", (unsigned)metadata->i2c_timing.read_ms);
  printf("i2c_timing_read_with_temp_comp_ms=%u\n",
         (unsigned)metadata->i2c_timing.read_with_temp_comp_ms);
  printf("i2c_timing_calibration_ms=%u\n", (unsigned)metadata->i2c_timing.calibration_ms);
}

static ezo_result_t print_product_readiness_i2c(ezo_i2c_device_t *device,
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

      I2C_QUERY(ezo_ph_send_temperature_query_i2c(device, &hint),
                      ezo_ph_read_temperature_i2c(device, &temperature));
      I2C_QUERY(ezo_ph_send_calibration_query_i2c(device, &hint),
                      ezo_ph_read_calibration_status_i2c(device, &calibration));
      I2C_QUERY(ezo_ph_send_slope_query_i2c(device, &hint),
                      ezo_ph_read_slope_i2c(device, &slope));
      I2C_QUERY(ezo_ph_send_extended_range_query_i2c(device, &hint),
                      ezo_ph_read_extended_range_i2c(device, &extended_range));

      printf("ph_temperature_compensation_c=%.3f\n", temperature.temperature_c);
      printf("ph_calibration_level=%s\n", ph_calibration_name(calibration.level));
      printf("ph_slope_acid_percent=%.3f\n", slope.acid_percent);
      printf("ph_slope_base_percent=%.3f\n", slope.base_percent);
      printf("ph_slope_neutral_mv=%.3f\n", slope.neutral_mv);
      printf("ph_extended_range=%s\n",
             bool_name(extended_range.enabled == EZO_PH_EXTENDED_RANGE_ENABLED));
      printf("readiness_supported=1\n");
      return EZO_OK;
    }
    case EZO_PRODUCT_ORP: {
      ezo_orp_calibration_status_t calibration;
      ezo_orp_extended_scale_status_t extended_scale;

      I2C_QUERY(ezo_orp_send_calibration_query_i2c(device, &hint),
                ezo_orp_read_calibration_status_i2c(device, &calibration));
      I2C_QUERY(ezo_orp_send_extended_scale_query_i2c(device, &hint),
                ezo_orp_read_extended_scale_i2c(device, &extended_scale));

      printf("orp_calibrated=%u\n", (unsigned)calibration.calibrated);
      printf("orp_extended_scale=%s\n", orp_extended_scale_name(extended_scale.enabled));
      printf("readiness_supported=1\n");
      return EZO_OK;
    }
    case EZO_PRODUCT_EC: {
      ezo_ec_output_config_t output_config;
      ezo_ec_temperature_compensation_t temperature;
      ezo_ec_probe_k_t probe_k;
      ezo_ec_tds_factor_t tds_factor;
      ezo_ec_calibration_status_t calibration;

      I2C_QUERY(ezo_ec_send_output_query_i2c(device, &hint),
                ezo_ec_read_output_config_i2c(device, &output_config));
      I2C_QUERY(ezo_ec_send_temperature_query_i2c(device, &hint),
                ezo_ec_read_temperature_i2c(device, &temperature));
      I2C_QUERY(ezo_ec_send_probe_k_query_i2c(device, &hint),
                ezo_ec_read_probe_k_i2c(device, &probe_k));
      I2C_QUERY(ezo_ec_send_tds_factor_query_i2c(device, &hint),
                ezo_ec_read_tds_factor_i2c(device, &tds_factor));
      I2C_QUERY(ezo_ec_send_calibration_query_i2c(device, &hint),
                ezo_ec_read_calibration_status_i2c(device, &calibration));

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

      I2C_QUERY(ezo_do_send_output_query_i2c(device, &hint),
                ezo_do_read_output_config_i2c(device, &output_config));
      I2C_QUERY(ezo_do_send_temperature_query_i2c(device, &hint),
                ezo_do_read_temperature_i2c(device, &temperature));
      I2C_QUERY(ezo_do_send_salinity_query_i2c(device, &hint),
                ezo_do_read_salinity_i2c(device, &salinity));
      I2C_QUERY(ezo_do_send_pressure_query_i2c(device, &hint),
                ezo_do_read_pressure_i2c(device, &pressure));
      I2C_QUERY(ezo_do_send_calibration_query_i2c(device, &hint),
                ezo_do_read_calibration_status_i2c(device, &calibration));

      printf("do_output_mask=%u\n", (unsigned)output_config.enabled_mask);
      printf("do_temperature_compensation_c=%.3f\n", temperature.temperature_c);
      printf("do_salinity_value=%.3f\n", salinity.value);
      printf("do_salinity_unit=%s\n", do_salinity_unit_name(salinity.unit));
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

      I2C_QUERY(ezo_rtd_send_scale_query_i2c(device, &hint),
                ezo_rtd_read_scale_i2c(device, &scale));
      I2C_QUERY(ezo_rtd_send_calibration_query_i2c(device, &hint),
                ezo_rtd_read_calibration_status_i2c(device, &calibration));
      I2C_QUERY(ezo_rtd_send_logger_query_i2c(device, &hint),
                ezo_rtd_read_logger_i2c(device, &logger));
      I2C_QUERY(ezo_rtd_send_memory_query_i2c(device, &hint),
                ezo_rtd_read_memory_status_i2c(device, &memory));

      printf("rtd_scale=%s\n", rtd_scale_name(scale.scale));
      printf("rtd_calibrated=%u\n", (unsigned)calibration.calibrated);
      printf("rtd_logger_interval_units=%u\n", (unsigned)logger.interval_units);
      printf("rtd_memory_last_index=%u\n", (unsigned)memory.last_index);
      printf("readiness_supported=1\n");
      return EZO_OK;
    }
    case EZO_PRODUCT_HUM: {
      ezo_hum_output_config_t output_config;
      ezo_hum_temperature_calibration_status_t calibration;

      I2C_QUERY(ezo_hum_send_output_query_i2c(device, &hint),
                ezo_hum_read_output_config_i2c(device, &output_config));
      I2C_QUERY(ezo_hum_send_temperature_calibration_query_i2c(device, &hint),
                ezo_hum_read_temperature_calibration_status_i2c(device, &calibration));

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

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_device_info_t info;
  const ezo_product_metadata_t *metadata = NULL;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 99U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_example_query_info_i2c(&session.device, &info);
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_info", result);
  }

  metadata = ezo_product_get_metadata(info.product_id);

  printf("transport=i2c\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("product=%s\n", metadata != NULL ? metadata->vendor_short_code : "unknown");
  print_device_info(&info);
  print_product_metadata(metadata);

  result = ezo_example_print_shared_control_i2c(&session.device, info.product_id);
  if (result == EZO_OK) {
    result = print_product_readiness_i2c(&session.device, info.product_id);
  }

  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("readiness_check", result);
  }

  return 0;
}
