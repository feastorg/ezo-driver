#ifndef EZO_ARDUINO_COMMON_HPP
#define EZO_ARDUINO_COMMON_HPP

#include <Arduino.h>

#include <ezo_control.h>
#include <ezo_do.h>
#include <ezo_ec.h>
#include <ezo_hum.h>
#include <ezo_orp.h>
#include <ezo_ph.h>
#include <ezo_product.h>
#include <ezo_rtd.h>

inline void ezo_arduino_fail_fast(const char *step, ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

  Serial.print(F("driver_error_step="));
  Serial.println(step);
  Serial.print(F("driver_error_name="));
  Serial.println(ezo_result_name(result));
  Serial.print(F("driver_error_code="));
  Serial.println((int)result);
  while (true) {
  }
}

#define EZO_ARDUINO_CHECK_OK(step, expr) ezo_arduino_fail_fast(step, (expr))

inline void ezo_arduino_wait_hint(const ezo_timing_hint_t *hint) {
  if (hint == NULL) {
    return;
  }
  delay(hint->wait_ms);
}

inline int ezo_arduino_startup_elapsed(unsigned long startup_started_at_ms,
                                       unsigned long startup_settle_ms) {
  return (unsigned long)(millis() - startup_started_at_ms) >= startup_settle_ms;
}

inline const char *ezo_arduino_bool_name(uint8_t value) {
  return value != 0U ? "enabled" : "disabled";
}

inline const char *ezo_arduino_rtd_scale_name(ezo_rtd_scale_t scale) {
  switch (scale) {
    case EZO_RTD_SCALE_CELSIUS:
      return "celsius";
    case EZO_RTD_SCALE_KELVIN:
      return "kelvin";
    case EZO_RTD_SCALE_FAHRENHEIT:
      return "fahrenheit";
    case EZO_RTD_SCALE_UNKNOWN:
    default:
      return "unknown";
  }
}

inline const char *ezo_arduino_do_salinity_unit_name(ezo_do_salinity_unit_t unit) {
  switch (unit) {
    case EZO_DO_SALINITY_UNIT_MICROSIEMENS:
      return "us_cm";
    case EZO_DO_SALINITY_UNIT_PPT:
      return "ppt";
    default:
      return "unknown";
  }
}

inline const char *ezo_arduino_ph_calibration_name(ezo_ph_calibration_level_t level) {
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

inline int ezo_arduino_rtd_reading_to_celsius(const ezo_rtd_reading_t *reading,
                                              float *temperature_c_out) {
  if (reading == NULL || temperature_c_out == NULL) {
    return 0;
  }

  switch (reading->scale) {
    case EZO_RTD_SCALE_CELSIUS:
      *temperature_c_out = (float)reading->temperature;
      return 1;
    case EZO_RTD_SCALE_KELVIN:
      *temperature_c_out = (float)(reading->temperature - 273.15);
      return 1;
    case EZO_RTD_SCALE_FAHRENHEIT:
      *temperature_c_out = (float)((reading->temperature - 32.0) * (5.0 / 9.0));
      return 1;
    case EZO_RTD_SCALE_UNKNOWN:
    default:
      return 0;
  }
}

inline void ezo_arduino_print_ec_reading(const __FlashStringHelper *prefix,
                                         const ezo_ec_reading_t *reading) {
  if (reading == NULL) {
    return;
  }

  if ((reading->present_mask & EZO_EC_OUTPUT_CONDUCTIVITY) != 0U) {
    Serial.print(prefix);
    Serial.print(F("conductivity_us_cm="));
    Serial.println(reading->conductivity_us_cm, 3);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS) != 0U) {
    Serial.print(prefix);
    Serial.print(F("total_dissolved_solids_ppm="));
    Serial.println(reading->total_dissolved_solids_ppm, 3);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SALINITY) != 0U) {
    Serial.print(prefix);
    Serial.print(F("salinity_ppt="));
    Serial.println(reading->salinity_ppt, 3);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SPECIFIC_GRAVITY) != 0U) {
    Serial.print(prefix);
    Serial.print(F("specific_gravity="));
    Serial.println(reading->specific_gravity, 3);
  }
}

inline void ezo_arduino_print_do_reading(const __FlashStringHelper *prefix,
                                         const ezo_do_reading_t *reading) {
  if (reading == NULL) {
    return;
  }

  if ((reading->present_mask & EZO_DO_OUTPUT_MG_L) != 0U) {
    Serial.print(prefix);
    Serial.print(F("milligrams_per_liter="));
    Serial.println(reading->milligrams_per_liter, 3);
  }
  if ((reading->present_mask & EZO_DO_OUTPUT_PERCENT_SATURATION) != 0U) {
    Serial.print(prefix);
    Serial.print(F("percent_saturation="));
    Serial.println(reading->percent_saturation, 3);
  }
}

inline void ezo_arduino_print_hum_reading(const __FlashStringHelper *prefix,
                                          const ezo_hum_reading_t *reading) {
  if (reading == NULL) {
    return;
  }

  if ((reading->present_mask & EZO_HUM_OUTPUT_HUMIDITY) != 0U) {
    Serial.print(prefix);
    Serial.print(F("relative_humidity_percent="));
    Serial.println(reading->relative_humidity_percent, 3);
  }
  if ((reading->present_mask & EZO_HUM_OUTPUT_AIR_TEMPERATURE) != 0U) {
    Serial.print(prefix);
    Serial.print(F("air_temperature_c="));
    Serial.println(reading->air_temperature_c, 3);
  }
  if ((reading->present_mask & EZO_HUM_OUTPUT_DEW_POINT) != 0U) {
    Serial.print(prefix);
    Serial.print(F("dew_point_c="));
    Serial.println(reading->dew_point_c, 3);
  }
}

inline const ezo_product_metadata_t *ezo_arduino_product_metadata(ezo_product_id_t product_id) {
  return ezo_product_get_metadata(product_id);
}

#endif
