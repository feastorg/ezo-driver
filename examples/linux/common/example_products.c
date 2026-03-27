#include "example_products.h"

#include <stdio.h>

const char *ezo_example_bool_name(int value) {
  return value ? "enabled" : "disabled";
}

const char *ezo_example_rtd_scale_name(ezo_rtd_scale_t scale) {
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

const char *ezo_example_do_salinity_unit_name(ezo_do_salinity_unit_t unit) {
  return unit == EZO_DO_SALINITY_UNIT_PPT ? "ppt" : "us_cm";
}

const char *ezo_example_ph_calibration_name(ezo_ph_calibration_level_t level) {
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

int ezo_example_rtd_reading_to_celsius(const ezo_rtd_reading_t *reading,
                                       double *temperature_c_out) {
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

void ezo_example_print_ec_reading(const char *prefix, const ezo_ec_reading_t *reading) {
  if (prefix == NULL || reading == NULL) {
    return;
  }

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

void ezo_example_print_do_reading(const char *prefix, const ezo_do_reading_t *reading) {
  if (prefix == NULL || reading == NULL) {
    return;
  }

  if ((reading->present_mask & EZO_DO_OUTPUT_MG_L) != 0U) {
    printf("%smilligrams_per_liter=%.3f\n", prefix, reading->milligrams_per_liter);
  }
  if ((reading->present_mask & EZO_DO_OUTPUT_PERCENT_SATURATION) != 0U) {
    printf("%spercent_saturation=%.3f\n", prefix, reading->percent_saturation);
  }
}

void ezo_example_print_hum_reading(const char *prefix, const ezo_hum_reading_t *reading) {
  if (prefix == NULL || reading == NULL) {
    return;
  }

  if ((reading->present_mask & EZO_HUM_OUTPUT_HUMIDITY) != 0U) {
    printf("%srelative_humidity_percent=%.3f\n", prefix, reading->relative_humidity_percent);
  }
  if ((reading->present_mask & EZO_HUM_OUTPUT_AIR_TEMPERATURE) != 0U) {
    printf("%sair_temperature_c=%.3f\n", prefix, reading->air_temperature_c);
  }
  if ((reading->present_mask & EZO_HUM_OUTPUT_DEW_POINT) != 0U) {
    printf("%sdew_point_c=%.3f\n", prefix, reading->dew_point_c);
  }
}
