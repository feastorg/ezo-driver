#ifndef EZO_SCHEMA_H
#define EZO_SCHEMA_H

#include "ezo.h"
#include "ezo_product.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EZO_SCHEMA_MAX_FIELDS 4

typedef enum {
  EZO_MEASUREMENT_FIELD_UNKNOWN = 0,
  EZO_MEASUREMENT_FIELD_PH,
  EZO_MEASUREMENT_FIELD_ORP,
  EZO_MEASUREMENT_FIELD_TEMPERATURE,
  EZO_MEASUREMENT_FIELD_CONDUCTIVITY,
  EZO_MEASUREMENT_FIELD_TOTAL_DISSOLVED_SOLIDS,
  EZO_MEASUREMENT_FIELD_SALINITY,
  EZO_MEASUREMENT_FIELD_SPECIFIC_GRAVITY,
  EZO_MEASUREMENT_FIELD_DISSOLVED_OXYGEN,
  EZO_MEASUREMENT_FIELD_OXYGEN_SATURATION,
  EZO_MEASUREMENT_FIELD_RELATIVE_HUMIDITY,
  EZO_MEASUREMENT_FIELD_DEW_POINT,
  EZO_MEASUREMENT_FIELD_AIR_TEMPERATURE
} ezo_measurement_field_t;

typedef struct {
  ezo_measurement_field_t field;
  double value;
  uint8_t present;
} ezo_output_value_t;

typedef struct {
  ezo_measurement_field_t field;
  double value;
  uint8_t present;
} ezo_scalar_reading_t;

typedef struct {
  ezo_product_id_t product_id;
  uint8_t field_count;
  ezo_measurement_field_t fields[EZO_SCHEMA_MAX_FIELDS];
} ezo_output_schema_t;

typedef struct {
  ezo_product_id_t product_id;
  uint8_t field_count;
  uint32_t present_mask;
  ezo_output_value_t values[EZO_SCHEMA_MAX_FIELDS];
} ezo_multi_output_reading_t;

ezo_result_t ezo_schema_get_output_schema(ezo_product_id_t product_id,
                                          ezo_output_schema_t *schema_out);

size_t ezo_schema_count_enabled_fields(const ezo_output_schema_t *schema,
                                       uint32_t enabled_mask);

ezo_result_t ezo_schema_parse_scalar_reading(const char *buffer,
                                             size_t buffer_len,
                                             ezo_measurement_field_t field,
                                             ezo_scalar_reading_t *reading_out);

ezo_result_t ezo_schema_parse_multi_output_reading(const char *buffer,
                                                   size_t buffer_len,
                                                   const ezo_output_schema_t *schema,
                                                   uint32_t enabled_mask,
                                                   ezo_multi_output_reading_t *reading_out);

#ifdef __cplusplus
}
#endif

#endif
