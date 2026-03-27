#ifndef EZO_EXAMPLE_PRODUCTS_H
#define EZO_EXAMPLE_PRODUCTS_H

#include "ezo_do.h"
#include "ezo_ec.h"
#include "ezo_hum.h"
#include "ezo_ph.h"
#include "ezo_rtd.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *ezo_example_bool_name(int value);

const char *ezo_example_rtd_scale_name(ezo_rtd_scale_t scale);

const char *ezo_example_do_salinity_unit_name(ezo_do_salinity_unit_t unit);

const char *ezo_example_ph_calibration_name(ezo_ph_calibration_level_t level);

int ezo_example_rtd_reading_to_celsius(const ezo_rtd_reading_t *reading,
                                       double *temperature_c_out);

void ezo_example_print_ec_reading(const char *prefix, const ezo_ec_reading_t *reading);

void ezo_example_print_do_reading(const char *prefix, const ezo_do_reading_t *reading);

void ezo_example_print_hum_reading(const char *prefix, const ezo_hum_reading_t *reading);

#ifdef __cplusplus
}
#endif

#endif
