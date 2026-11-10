#ifndef EZO_PRODUCT_H
#define EZO_PRODUCT_H

#include "ezo.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EZO_PRODUCT_SHORT_CODE_MAX_LEN 8
#define EZO_PRODUCT_FIRMWARE_VERSION_MAX_LEN 16

typedef enum {
  EZO_PRODUCT_UNKNOWN = 0,
  EZO_PRODUCT_PH,
  EZO_PRODUCT_ORP,
  EZO_PRODUCT_EC,
  EZO_PRODUCT_DO,
  EZO_PRODUCT_RTD,
  EZO_PRODUCT_HUM,
  EZO_PRODUCT_CO2,
  EZO_PRODUCT_O2,
  EZO_PRODUCT_RGB,
  EZO_PRODUCT_PRS,
  EZO_PRODUCT_FLO,
  EZO_PRODUCT_PMP,
  EZO_PRODUCT_PMP_L
} ezo_product_id_t;

typedef enum {
  EZO_PRODUCT_SUPPORT_UNKNOWN = 0,
  EZO_PRODUCT_SUPPORT_METADATA,
  EZO_PRODUCT_SUPPORT_TYPED_READ,
  EZO_PRODUCT_SUPPORT_FULL
} ezo_product_support_t;

typedef enum {
  EZO_PRODUCT_TRANSPORT_UNKNOWN = 0,
  EZO_PRODUCT_TRANSPORT_UART,
  EZO_PRODUCT_TRANSPORT_I2C
} ezo_product_transport_t;

typedef enum {
  EZO_PRODUCT_DEFAULT_UNKNOWN = 0,
  EZO_PRODUCT_DEFAULT_DISABLED,
  EZO_PRODUCT_DEFAULT_ENABLED,
  EZO_PRODUCT_DEFAULT_QUERY_REQUIRED
} ezo_product_default_state_t;

typedef enum {
  EZO_PRODUCT_OUTPUT_SCHEMA_UNKNOWN = 0,
  EZO_PRODUCT_OUTPUT_SCHEMA_SCALAR_SINGLE,
  EZO_PRODUCT_OUTPUT_SCHEMA_PRIMARY_ONLY,
  EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED
} ezo_product_output_schema_t;

enum {
  EZO_PRODUCT_CAP_MEASUREMENT = 1u << 0,
  EZO_PRODUCT_CAP_CALIBRATION = 1u << 1,
  EZO_PRODUCT_CAP_TEMPERATURE_COMPENSATION = 1u << 2,
  EZO_PRODUCT_CAP_SALINITY_COMPENSATION = 1u << 3,
  EZO_PRODUCT_CAP_PRESSURE_COMPENSATION = 1u << 4,
  EZO_PRODUCT_CAP_OUTPUT_SELECTION = 1u << 5,
  EZO_PRODUCT_CAP_CALIBRATION_TRANSFER = 1u << 6,
  EZO_PRODUCT_CAP_DATA_LOGGING = 1u << 7,
  EZO_PRODUCT_CAP_MEMORY_RECALL = 1u << 8,
  EZO_PRODUCT_CAP_RANGE_SELECTION = 1u << 9,
  EZO_PRODUCT_CAP_PROBE_CONFIGURATION = 1u << 10,
  EZO_PRODUCT_CAP_TDS_CONFIGURATION = 1u << 11,
  EZO_PRODUCT_CAP_SCALE_SELECTION = 1u << 12
};

enum {
  EZO_PRODUCT_FAMILY_ACQUISITION = 1u << 0,
  EZO_PRODUCT_FAMILY_CALIBRATION = 1u << 1,
  EZO_PRODUCT_FAMILY_CALIBRATION_TRANSFER = 1u << 2,
  EZO_PRODUCT_FAMILY_IDENTITY = 1u << 3,
  EZO_PRODUCT_FAMILY_DEVICE_CONTROL = 1u << 4,
  EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL = 1u << 5
};

typedef struct {
  uint32_t generic_ms;
  uint32_t read_ms;
  uint32_t read_with_temp_comp_ms;
  uint32_t calibration_ms;
} ezo_product_timing_profile_t;

typedef struct {
  ezo_product_id_t product_id;
  char product_code[EZO_PRODUCT_SHORT_CODE_MAX_LEN];
  char firmware_version[EZO_PRODUCT_FIRMWARE_VERSION_MAX_LEN];
} ezo_device_info_t;

/* A firmware version as a decimal: "1.1" and "1.10" are both {1, 10}, and "1.01" is {1, 1}. */
typedef struct {
  uint16_t major;
  uint16_t minor_hundredths;
} ezo_firmware_version_t;

typedef enum {
  EZO_FIRMWARE_FEATURE_UNKNOWN = 0,
  EZO_FIRMWARE_FEATURE_FIND,
  EZO_FIRMWARE_FEATURE_CALIBRATION_TRANSFER,
  EZO_FIRMWARE_FEATURE_PROTOCOL_LOCK,
  EZO_FIRMWARE_FEATURE_READ_WITH_TEMP_COMP,
  EZO_FIRMWARE_FEATURE_EXTENDED_RANGE,
  EZO_FIRMWARE_FEATURE_TEMPERATURE_CALIBRATION
} ezo_firmware_feature_t;

typedef enum {
  EZO_FIRMWARE_ISSUE_UNKNOWN = 0,
  EZO_FIRMWARE_ISSUE_I2C_SLEEP_WAKE,
  EZO_FIRMWARE_ISSUE_FIRST_READING_INVALID
} ezo_firmware_issue_t;

typedef struct {
  ezo_product_id_t product_id;
  const char *family_name;
  const char *vendor_short_code;
  ezo_product_support_t support_tier;
  ezo_product_transport_t default_transport;
  uint8_t default_i2c_address;
  ezo_product_default_state_t default_continuous_mode;
  ezo_product_default_state_t default_response_codes;
  ezo_product_output_schema_t default_output_schema;
  uint8_t default_output_count;
  uint32_t capability_flags;
  uint32_t command_family_flags;
  ezo_product_timing_profile_t uart_timing;
  ezo_product_timing_profile_t i2c_timing;
} ezo_product_metadata_t;

ezo_result_t ezo_product_id_from_short_code(const char *short_code,
                                            size_t short_code_len,
                                            ezo_product_id_t *product_id);

ezo_result_t ezo_parse_device_info(const char *buffer,
                                   size_t buffer_len,
                                   ezo_device_info_t *device_info);

const ezo_product_metadata_t *ezo_product_get_metadata(ezo_product_id_t product_id);

const ezo_product_metadata_t *ezo_product_get_metadata_by_short_code(const char *short_code,
                                                                     size_t short_code_len);

ezo_result_t ezo_product_get_timing_hint(ezo_product_id_t product_id,
                                         ezo_product_transport_t transport,
                                         ezo_command_kind_t kind,
                                         ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_product_resolve_timing_hint(ezo_product_id_t product_id,
                                             ezo_product_transport_t transport,
                                             ezo_command_kind_t kind,
                                             ezo_timing_hint_t *timing_hint);

ezo_product_support_t ezo_product_get_support_tier(ezo_product_id_t product_id);

int ezo_product_supports_capability(ezo_product_id_t product_id, uint32_t capability_flag);

int ezo_product_has_command_family(ezo_product_id_t product_id, uint32_t family_flag);

ezo_result_t ezo_parse_firmware_version(const char *text,
                                        size_t text_len,
                                        ezo_firmware_version_t *version);

/* Returns a negative value, 0, or a positive value as left is below, equal to, or above right. */
int ezo_firmware_version_compare(const ezo_firmware_version_t *left,
                                 const ezo_firmware_version_t *right);

/* Facts from the vendor firmware changelogs, for the FULL-tier products only. No send helper
 * consults these; the caller decides. */
int ezo_product_firmware_has_feature(ezo_product_id_t product_id,
                                     ezo_firmware_feature_t feature,
                                     const ezo_firmware_version_t *version);

int ezo_product_firmware_has_issue(ezo_product_id_t product_id,
                                   ezo_firmware_issue_t issue,
                                   const ezo_firmware_version_t *version);

#ifdef __cplusplus
}
#endif

#endif
