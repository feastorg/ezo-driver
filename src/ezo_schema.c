#include "ezo_schema.h"

#include "ezo_parse.h"

typedef struct {
  ezo_product_id_t product_id;
  uint8_t field_count;
  ezo_measurement_field_t fields[EZO_SCHEMA_MAX_FIELDS];
} ezo_schema_registry_entry_t;

static const ezo_schema_registry_entry_t ezo_schema_registry[] = {
    {EZO_PRODUCT_PH, 1, {EZO_MEASUREMENT_FIELD_PH}},
    {EZO_PRODUCT_ORP, 1, {EZO_MEASUREMENT_FIELD_ORP}},
    {EZO_PRODUCT_EC,
     4,
     {EZO_MEASUREMENT_FIELD_CONDUCTIVITY,
      EZO_MEASUREMENT_FIELD_TOTAL_DISSOLVED_SOLIDS,
      EZO_MEASUREMENT_FIELD_SALINITY,
      EZO_MEASUREMENT_FIELD_SPECIFIC_GRAVITY}},
    {EZO_PRODUCT_DO,
     2,
     {EZO_MEASUREMENT_FIELD_DISSOLVED_OXYGEN, EZO_MEASUREMENT_FIELD_OXYGEN_SATURATION}},
    {EZO_PRODUCT_RTD, 1, {EZO_MEASUREMENT_FIELD_TEMPERATURE}},
    {EZO_PRODUCT_HUM,
     3,
     {EZO_MEASUREMENT_FIELD_RELATIVE_HUMIDITY,
      EZO_MEASUREMENT_FIELD_DEW_POINT,
      EZO_MEASUREMENT_FIELD_AIR_TEMPERATURE}},
};

static const ezo_schema_registry_entry_t *ezo_schema_find_entry(
    ezo_product_id_t product_id) {
  size_t i = 0;

  for (i = 0; i < sizeof(ezo_schema_registry) / sizeof(ezo_schema_registry[0]); ++i) {
    if (ezo_schema_registry[i].product_id == product_id) {
      return &ezo_schema_registry[i];
    }
  }

  return NULL;
}

ezo_result_t ezo_schema_get_output_schema(ezo_product_id_t product_id,
                                          ezo_output_schema_t *schema_out) {
  const ezo_schema_registry_entry_t *entry = NULL;
  size_t i = 0;

  if (schema_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  entry = ezo_schema_find_entry(product_id);
  if (entry == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  schema_out->product_id = entry->product_id;
  schema_out->field_count = entry->field_count;
  for (i = 0; i < EZO_SCHEMA_MAX_FIELDS; ++i) {
    schema_out->fields[i] = entry->fields[i];
  }

  return EZO_OK;
}

size_t ezo_schema_count_enabled_fields(const ezo_output_schema_t *schema,
                                       uint32_t enabled_mask) {
  size_t count = 0;
  size_t i = 0;

  if (schema == NULL || schema->field_count == 0 || schema->field_count > EZO_SCHEMA_MAX_FIELDS) {
    return 0;
  }

  for (i = 0; i < schema->field_count; ++i) {
    if ((enabled_mask & (1u << i)) != 0u) {
      count += 1;
    }
  }

  return count;
}

ezo_result_t ezo_schema_parse_scalar_reading(const char *buffer,
                                             size_t buffer_len,
                                             ezo_measurement_field_t field,
                                             ezo_scalar_reading_t *reading_out) {
  double value = 0.0;
  ezo_result_t result = EZO_OK;

  if (buffer == NULL || buffer_len == 0 || field == EZO_MEASUREMENT_FIELD_UNKNOWN ||
      reading_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_double(buffer, buffer_len, &value);
  if (result != EZO_OK) {
    return result;
  }

  reading_out->field = field;
  reading_out->value = value;
  reading_out->present = 1;
  return EZO_OK;
}

ezo_result_t ezo_schema_parse_multi_output_reading(const char *buffer,
                                                   size_t buffer_len,
                                                   const ezo_output_schema_t *schema,
                                                   uint32_t enabled_mask,
                                                   ezo_multi_output_reading_t *reading_out) {
  ezo_text_span_t fields[EZO_SCHEMA_MAX_FIELDS];
  size_t expected_field_count = 0;
  size_t parsed_field_count = 0;
  size_t next_value_index = 0;
  size_t i = 0;
  uint32_t valid_mask = 0;
  ezo_result_t result = EZO_OK;

  if (buffer == NULL || buffer_len == 0 || schema == NULL || reading_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (schema->field_count == 0 || schema->field_count > EZO_SCHEMA_MAX_FIELDS) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  valid_mask = (1u << schema->field_count) - 1u;
  enabled_mask &= valid_mask;
  expected_field_count = ezo_schema_count_enabled_fields(schema, enabled_mask);
  if (expected_field_count == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_csv_fields(buffer,
                                buffer_len,
                                fields,
                                EZO_SCHEMA_MAX_FIELDS,
                                &parsed_field_count);
  if (result != EZO_OK) {
    return result;
  }

  if (parsed_field_count != expected_field_count) {
    return EZO_ERR_PARSE;
  }

  reading_out->product_id = schema->product_id;
  reading_out->field_count = schema->field_count;
  reading_out->present_mask = enabled_mask;
  for (i = 0; i < EZO_SCHEMA_MAX_FIELDS; ++i) {
    reading_out->values[i].field = (i < schema->field_count) ? schema->fields[i]
                                                             : EZO_MEASUREMENT_FIELD_UNKNOWN;
    reading_out->values[i].value = 0.0;
    reading_out->values[i].present = 0;
  }

  for (i = 0; i < schema->field_count; ++i) {
    if ((enabled_mask & (1u << i)) == 0u) {
      continue;
    }

    result = ezo_parse_text_span_double(fields[next_value_index],
                                        &reading_out->values[i].value);
    if (result != EZO_OK) {
      return result;
    }

    reading_out->values[i].present = 1;
    next_value_index += 1;
  }

  return EZO_OK;
}
