#include "ezo_product.h"

#include <stddef.h>
#include <string.h>

typedef struct {
  const char *normalized_code;
  const ezo_product_metadata_t metadata;
} ezo_product_registry_entry_t;

static const ezo_product_registry_entry_t ezo_product_registry[] = {
    {
        "PH",
        {
            EZO_PRODUCT_PH,
            "pH",
            "pH",
            EZO_PRODUCT_SUPPORT_TYPED_READ,
            EZO_PRODUCT_TRANSPORT_UART,
            99,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_OUTPUT_SCHEMA_SCALAR_SINGLE,
            1,
            EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION |
                EZO_PRODUCT_CAP_TEMPERATURE_COMPENSATION |
                EZO_PRODUCT_CAP_CALIBRATION_TRANSFER | EZO_PRODUCT_CAP_RANGE_SELECTION,
            EZO_PRODUCT_FAMILY_ACQUISITION | EZO_PRODUCT_FAMILY_CALIBRATION |
                EZO_PRODUCT_FAMILY_CALIBRATION_TRANSFER | EZO_PRODUCT_FAMILY_IDENTITY |
                EZO_PRODUCT_FAMILY_DEVICE_CONTROL | EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL,
            {300U, 900U, 900U, 900U},
            {300U, 900U, 900U, 900U},
        },
    },
    {
        "ORP",
        {
            EZO_PRODUCT_ORP,
            "ORP",
            "ORP",
            EZO_PRODUCT_SUPPORT_TYPED_READ,
            EZO_PRODUCT_TRANSPORT_UART,
            98,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_OUTPUT_SCHEMA_SCALAR_SINGLE,
            1,
            EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION |
                EZO_PRODUCT_CAP_CALIBRATION_TRANSFER | EZO_PRODUCT_CAP_RANGE_SELECTION,
            EZO_PRODUCT_FAMILY_ACQUISITION | EZO_PRODUCT_FAMILY_CALIBRATION |
                EZO_PRODUCT_FAMILY_CALIBRATION_TRANSFER | EZO_PRODUCT_FAMILY_IDENTITY |
                EZO_PRODUCT_FAMILY_DEVICE_CONTROL | EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL,
            {300U, 1000U, 0U, 900U},
            {300U, 1000U, 0U, 900U},
        },
    },
    {
        "EC",
        {
            EZO_PRODUCT_EC,
            "EC",
            "EC",
            EZO_PRODUCT_SUPPORT_TYPED_READ,
            EZO_PRODUCT_TRANSPORT_UART,
            100,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_DEFAULT_ENABLED,
            /* Vendor output defaults have changed across revisions. */
            EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED,
            0,
            EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION |
                EZO_PRODUCT_CAP_TEMPERATURE_COMPENSATION |
                EZO_PRODUCT_CAP_OUTPUT_SELECTION | EZO_PRODUCT_CAP_CALIBRATION_TRANSFER |
                EZO_PRODUCT_CAP_PROBE_CONFIGURATION | EZO_PRODUCT_CAP_TDS_CONFIGURATION,
            EZO_PRODUCT_FAMILY_ACQUISITION | EZO_PRODUCT_FAMILY_CALIBRATION |
                EZO_PRODUCT_FAMILY_CALIBRATION_TRANSFER | EZO_PRODUCT_FAMILY_IDENTITY |
                EZO_PRODUCT_FAMILY_DEVICE_CONTROL | EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL,
            {300U, 600U, 900U, 1200U},
            {300U, 600U, 900U, 1200U},
        },
    },
    {
        "DO",
        {
            EZO_PRODUCT_DO,
            "Dissolved Oxygen",
            "D.O.",
            EZO_PRODUCT_SUPPORT_TYPED_READ,
            EZO_PRODUCT_TRANSPORT_UART,
            97,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_OUTPUT_SCHEMA_PRIMARY_ONLY,
            1,
            EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION |
                EZO_PRODUCT_CAP_TEMPERATURE_COMPENSATION |
                EZO_PRODUCT_CAP_SALINITY_COMPENSATION |
                EZO_PRODUCT_CAP_PRESSURE_COMPENSATION |
                EZO_PRODUCT_CAP_OUTPUT_SELECTION | EZO_PRODUCT_CAP_CALIBRATION_TRANSFER,
            EZO_PRODUCT_FAMILY_ACQUISITION | EZO_PRODUCT_FAMILY_CALIBRATION |
                EZO_PRODUCT_FAMILY_CALIBRATION_TRANSFER | EZO_PRODUCT_FAMILY_IDENTITY |
                EZO_PRODUCT_FAMILY_DEVICE_CONTROL | EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL,
            {300U, 600U, 900U, 1300U},
            {300U, 600U, 900U, 1300U},
        },
    },
    {
        "RTD",
        {
            EZO_PRODUCT_RTD,
            "RTD",
            "RTD",
            EZO_PRODUCT_SUPPORT_TYPED_READ,
            EZO_PRODUCT_TRANSPORT_UART,
            102,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_OUTPUT_SCHEMA_SCALAR_SINGLE,
            1,
            EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION |
                EZO_PRODUCT_CAP_CALIBRATION_TRANSFER | EZO_PRODUCT_CAP_DATA_LOGGING |
                EZO_PRODUCT_CAP_MEMORY_RECALL | EZO_PRODUCT_CAP_SCALE_SELECTION,
            EZO_PRODUCT_FAMILY_ACQUISITION | EZO_PRODUCT_FAMILY_CALIBRATION |
                EZO_PRODUCT_FAMILY_CALIBRATION_TRANSFER | EZO_PRODUCT_FAMILY_IDENTITY |
                EZO_PRODUCT_FAMILY_DEVICE_CONTROL | EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL,
            {300U, 1000U, 0U, 600U},
            {300U, 600U, 0U, 600U},
        },
    },
    {
        "HUM",
        {
            EZO_PRODUCT_HUM,
            "Humidity",
            "HUM",
            EZO_PRODUCT_SUPPORT_TYPED_READ,
            EZO_PRODUCT_TRANSPORT_UART,
            111,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_DEFAULT_ENABLED,
            EZO_PRODUCT_OUTPUT_SCHEMA_PRIMARY_ONLY,
            1,
            EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION |
                EZO_PRODUCT_CAP_OUTPUT_SELECTION,
            EZO_PRODUCT_FAMILY_ACQUISITION | EZO_PRODUCT_FAMILY_CALIBRATION |
                EZO_PRODUCT_FAMILY_IDENTITY | EZO_PRODUCT_FAMILY_DEVICE_CONTROL |
                EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL,
            {300U, 1000U, 0U, 300U},
            {300U, 300U, 0U, 300U},
        },
    },
};

enum {
  EZO_PRODUCT_NORMALIZED_CODE_MAX_LEN = 8
};

static size_t ezo_product_registry_count(void) {
  return sizeof(ezo_product_registry) / sizeof(ezo_product_registry[0]);
}

static int ezo_product_is_space(char value) {
  return value == ' ' || value == '\t' || value == '\n' || value == '\r' ||
         value == '\f' || value == '\v';
}

static char ezo_product_ascii_upper(char value) {
  if (value >= 'a' && value <= 'z') {
    return (char)(value - ('a' - 'A'));
  }
  return value;
}

static void ezo_product_trim_range(const char *buffer,
                                   size_t buffer_len,
                                   size_t *start_out,
                                   size_t *end_out) {
  size_t start = 0;
  size_t end = buffer_len;

  while (start < end && ezo_product_is_space(buffer[start])) {
    start += 1;
  }

  while (end > start && ezo_product_is_space(buffer[end - 1U])) {
    end -= 1;
  }

  *start_out = start;
  *end_out = end;
}

static ezo_result_t ezo_product_copy_trimmed_token(char *destination,
                                                   size_t destination_len,
                                                   const char *buffer,
                                                   size_t buffer_len) {
  size_t start = 0;
  size_t end = 0;
  size_t trimmed_len = 0;
  size_t i = 0;

  if (destination == NULL || destination_len == 0 || buffer == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  ezo_product_trim_range(buffer, buffer_len, &start, &end);
  trimmed_len = end - start;
  if (trimmed_len == 0 || trimmed_len >= destination_len) {
    return EZO_ERR_PARSE;
  }

  for (i = 0; i < trimmed_len; ++i) {
    destination[i] = buffer[start + i];
  }
  destination[trimmed_len] = '\0';
  return EZO_OK;
}

static ezo_result_t ezo_product_normalize_short_code(char *destination,
                                                     size_t destination_len,
                                                     const char *short_code,
                                                     size_t short_code_len) {
  size_t start = 0;
  size_t end = 0;
  size_t used = 0;
  size_t i = 0;

  if (destination == NULL || destination_len == 0 || short_code == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  ezo_product_trim_range(short_code, short_code_len, &start, &end);
  for (i = start; i < end; ++i) {
    char value = short_code[i];

    if (value == '.') {
      continue;
    }

    if (ezo_product_is_space(value)) {
      continue;
    }

    if (used + 1U >= destination_len) {
      return EZO_ERR_BUFFER_TOO_SMALL;
    }

    destination[used] = ezo_product_ascii_upper(value);
    used += 1;
  }

  if (used == 0) {
    return EZO_ERR_PARSE;
  }

  destination[used] = '\0';
  return EZO_OK;
}

static int ezo_product_streq(const char *left, const char *right) {
  size_t i = 0;

  if (left == NULL || right == NULL) {
    return 0;
  }

  while (left[i] != '\0' && right[i] != '\0') {
    if (left[i] != right[i]) {
      return 0;
    }
    i += 1;
  }

  return left[i] == '\0' && right[i] == '\0';
}

static const ezo_product_registry_entry_t *ezo_product_find_registry_entry(
    ezo_product_id_t product_id) {
  size_t i = 0;

  for (i = 0; i < ezo_product_registry_count(); ++i) {
    if (ezo_product_registry[i].metadata.product_id == product_id) {
      return &ezo_product_registry[i];
    }
  }

  return NULL;
}

static const ezo_product_registry_entry_t *ezo_product_find_registry_entry_by_code(
    const char *normalized_code) {
  size_t i = 0;

  if (normalized_code == NULL) {
    return NULL;
  }

  for (i = 0; i < ezo_product_registry_count(); ++i) {
    if (ezo_product_streq(ezo_product_registry[i].normalized_code, normalized_code)) {
      return &ezo_product_registry[i];
    }
  }

  return NULL;
}

ezo_result_t ezo_product_id_from_short_code(const char *short_code,
                                            size_t short_code_len,
                                            ezo_product_id_t *product_id) {
  char normalized_code[EZO_PRODUCT_NORMALIZED_CODE_MAX_LEN];
  const ezo_product_registry_entry_t *entry = NULL;
  ezo_result_t result = EZO_OK;

  if (short_code == NULL || short_code_len == 0 || product_id == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_product_normalize_short_code(normalized_code,
                                            sizeof(normalized_code),
                                            short_code,
                                            short_code_len);
  if (result != EZO_OK) {
    return result;
  }

  entry = ezo_product_find_registry_entry_by_code(normalized_code);
  *product_id = entry != NULL ? entry->metadata.product_id : EZO_PRODUCT_UNKNOWN;
  return EZO_OK;
}

ezo_result_t ezo_parse_device_info(const char *buffer,
                                   size_t buffer_len,
                                   ezo_device_info_t *device_info) {
  size_t start = 0;
  size_t end = 0;
  size_t first_comma = 0;
  size_t second_comma = 0;
  size_t i = 0;
  int saw_first_comma = 0;
  int saw_second_comma = 0;
  ezo_result_t result = EZO_OK;

  if (buffer == NULL || buffer_len == 0 || device_info == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  ezo_product_trim_range(buffer, buffer_len, &start, &end);
  if (end <= start) {
    return EZO_ERR_PARSE;
  }

  first_comma = end;
  second_comma = end;
  for (i = start; i < end; ++i) {
    if (buffer[i] != ',') {
      continue;
    }

    if (!saw_first_comma) {
      saw_first_comma = 1;
      first_comma = i;
      continue;
    }

    if (!saw_second_comma) {
      saw_second_comma = 1;
      second_comma = i;
      continue;
    }

    return EZO_ERR_PARSE;
  }

  if (!saw_first_comma || !saw_second_comma) {
    return EZO_ERR_PARSE;
  }

  {
    size_t prefix_start = 0;
    size_t prefix_end = 0;
    size_t prefix_len = 0;

    ezo_product_trim_range(buffer + start, first_comma - start, &prefix_start, &prefix_end);
    prefix_len = prefix_end - prefix_start;
    if (!((prefix_len == 2 &&
           buffer[start + prefix_start] == '?' &&
           buffer[start + prefix_start + 1U] == 'i') ||
          (prefix_len == 1 && buffer[start + prefix_start] == 'i'))) {
      return EZO_ERR_PARSE;
    }
  }

  result = ezo_product_copy_trimmed_token(device_info->product_code,
                                          sizeof(device_info->product_code),
                                          buffer + first_comma + 1U,
                                          second_comma - first_comma - 1U);
  if (result != EZO_OK) {
    return result;
  }

  result = ezo_product_copy_trimmed_token(device_info->firmware_version,
                                          sizeof(device_info->firmware_version),
                                          buffer + second_comma + 1U,
                                          end - second_comma - 1U);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_product_id_from_short_code(device_info->product_code,
                                        strlen(device_info->product_code),
                                        &device_info->product_id);
}

const ezo_product_metadata_t *ezo_product_get_metadata(ezo_product_id_t product_id) {
  const ezo_product_registry_entry_t *entry = ezo_product_find_registry_entry(product_id);
  return entry != NULL ? &entry->metadata : NULL;
}

const ezo_product_metadata_t *ezo_product_get_metadata_by_short_code(const char *short_code,
                                                                     size_t short_code_len) {
  ezo_product_id_t product_id = EZO_PRODUCT_UNKNOWN;
  ezo_result_t result = ezo_product_id_from_short_code(short_code, short_code_len, &product_id);
  if (result != EZO_OK || product_id == EZO_PRODUCT_UNKNOWN) {
    return NULL;
  }

  return ezo_product_get_metadata(product_id);
}

ezo_result_t ezo_product_get_timing_hint(ezo_product_id_t product_id,
                                         ezo_product_transport_t transport,
                                         ezo_command_kind_t kind,
                                         ezo_timing_hint_t *timing_hint) {
  const ezo_product_metadata_t *metadata = ezo_product_get_metadata(product_id);
  const ezo_product_timing_profile_t *profile = NULL;
  uint32_t wait_ms = 0;

  if (metadata == NULL || timing_hint == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  switch (transport) {
  case EZO_PRODUCT_TRANSPORT_UART:
    profile = &metadata->uart_timing;
    break;
  case EZO_PRODUCT_TRANSPORT_I2C:
    profile = &metadata->i2c_timing;
    break;
  case EZO_PRODUCT_TRANSPORT_UNKNOWN:
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }

  switch (kind) {
  case EZO_COMMAND_GENERIC:
    wait_ms = profile->generic_ms;
    break;
  case EZO_COMMAND_READ:
    wait_ms = profile->read_ms;
    break;
  case EZO_COMMAND_READ_WITH_TEMP_COMP:
    wait_ms = profile->read_with_temp_comp_ms;
    break;
  case EZO_COMMAND_CALIBRATION:
    wait_ms = profile->calibration_ms;
    break;
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (wait_ms == 0U) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  timing_hint->wait_ms = wait_ms;
  return EZO_OK;
}

ezo_result_t ezo_product_resolve_timing_hint(ezo_product_id_t product_id,
                                             ezo_product_transport_t transport,
                                             ezo_command_kind_t kind,
                                             ezo_timing_hint_t *timing_hint) {
  ezo_result_t result = EZO_OK;

  if (timing_hint == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (product_id != EZO_PRODUCT_UNKNOWN && transport != EZO_PRODUCT_TRANSPORT_UNKNOWN) {
    result = ezo_product_get_timing_hint(product_id, transport, kind, timing_hint);
    if (result == EZO_OK) {
      return EZO_OK;
    }
  }

  return ezo_get_timing_hint_for_command_kind(kind, timing_hint);
}

ezo_product_support_t ezo_product_get_support_tier(ezo_product_id_t product_id) {
  const ezo_product_metadata_t *metadata = ezo_product_get_metadata(product_id);
  return metadata != NULL ? metadata->support_tier : EZO_PRODUCT_SUPPORT_UNKNOWN;
}

int ezo_product_supports_capability(ezo_product_id_t product_id, uint32_t capability_flag) {
  const ezo_product_metadata_t *metadata = ezo_product_get_metadata(product_id);
  if (metadata == NULL || capability_flag == 0U) {
    return 0;
  }

  return (metadata->capability_flags & capability_flag) == capability_flag;
}

int ezo_product_has_command_family(ezo_product_id_t product_id, uint32_t family_flag) {
  const ezo_product_metadata_t *metadata = ezo_product_get_metadata(product_id);
  if (metadata == NULL || family_flag == 0U) {
    return 0;
  }

  return (metadata->command_family_flags & family_flag) == family_flag;
}
