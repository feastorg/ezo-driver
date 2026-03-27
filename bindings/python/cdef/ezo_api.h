typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t;

struct termios { ...; };

typedef enum {
  EZO_OK = 0,
  EZO_ERR_INVALID_ARGUMENT,
  EZO_ERR_BUFFER_TOO_SMALL,
  EZO_ERR_TRANSPORT,
  EZO_ERR_PROTOCOL,
  EZO_ERR_PARSE
} ezo_result_t;

typedef enum {
  EZO_COMMAND_GENERIC = 0,
  EZO_COMMAND_READ,
  EZO_COMMAND_READ_WITH_TEMP_COMP,
  EZO_COMMAND_CALIBRATION
} ezo_command_kind_t;

typedef struct {
  uint32_t wait_ms;
} ezo_timing_hint_t;

ezo_result_t ezo_get_timing_hint_for_command_kind(ezo_command_kind_t kind,
                                                  ezo_timing_hint_t *timing_hint);
const char *ezo_result_name(ezo_result_t result);
ezo_result_t ezo_parse_double(const char *buffer, size_t buffer_len, double *value_out);
ezo_result_t ezo_common_format_fixed_command(char *buffer,
                                             size_t buffer_len,
                                             const char *prefix,
                                             double value,
                                             uint8_t decimals);

#define EZO_I2C_MAX_RESPONSE_PAYLOAD_LEN ...
#define EZO_I2C_MAX_TEXT_RESPONSE_LEN ...
#define EZO_I2C_MAX_TEXT_RESPONSE_CAPACITY ...

typedef enum {
  EZO_STATUS_UNKNOWN = 0,
  EZO_STATUS_SUCCESS,
  EZO_STATUS_FAIL,
  EZO_STATUS_NOT_READY,
  EZO_STATUS_NO_DATA
} ezo_device_status_t;

typedef struct ezo_i2c_transport ezo_i2c_transport_t;

typedef struct {
  uint8_t address;
  const ezo_i2c_transport_t *transport;
  void *transport_context;
  uint8_t last_device_status;
} ezo_i2c_device_t;

ezo_result_t ezo_device_init(ezo_i2c_device_t *device,
                             uint8_t address,
                             const ezo_i2c_transport_t *transport,
                             void *transport_context);
void ezo_device_set_address(ezo_i2c_device_t *device, uint8_t address);
uint8_t ezo_device_get_address(const ezo_i2c_device_t *device);
ezo_device_status_t ezo_device_get_last_status(const ezo_i2c_device_t *device);
const char *ezo_device_status_name(ezo_device_status_t status);
ezo_result_t ezo_send_command(ezo_i2c_device_t *device,
                              const char *command,
                              ezo_command_kind_t kind,
                              ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_send_command_with_float(ezo_i2c_device_t *device,
                                         const char *prefix,
                                         double value,
                                         uint8_t decimals,
                                         ezo_command_kind_t kind,
                                         ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_send_read(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_send_read_with_temp_comp(ezo_i2c_device_t *device,
                                          double temperature_c,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_read_response_raw(ezo_i2c_device_t *device,
                                   uint8_t *buffer,
                                   size_t buffer_len,
                                   size_t *response_len,
                                   ezo_device_status_t *device_status);
ezo_result_t ezo_read_response(ezo_i2c_device_t *device,
                               char *buffer,
                               size_t buffer_len,
                               size_t *response_len,
                               ezo_device_status_t *device_status);

#define EZO_UART_MAX_TEXT_RESPONSE_LEN ...
#define EZO_UART_MAX_TEXT_RESPONSE_CAPACITY ...

typedef enum {
  EZO_UART_RESPONSE_UNKNOWN = 0,
  EZO_UART_RESPONSE_DATA,
  EZO_UART_RESPONSE_OK,
  EZO_UART_RESPONSE_ERROR,
  EZO_UART_RESPONSE_OVER_VOLTAGE,
  EZO_UART_RESPONSE_UNDER_VOLTAGE,
  EZO_UART_RESPONSE_RESET,
  EZO_UART_RESPONSE_READY,
  EZO_UART_RESPONSE_SLEEP,
  EZO_UART_RESPONSE_WAKE,
  EZO_UART_RESPONSE_DONE
} ezo_uart_response_kind_t;

typedef struct ezo_uart_transport ezo_uart_transport_t;

typedef struct {
  const ezo_uart_transport_t *transport;
  void *transport_context;
  uint8_t last_response_kind;
} ezo_uart_device_t;

ezo_result_t ezo_uart_device_init(ezo_uart_device_t *device,
                                  const ezo_uart_transport_t *transport,
                                  void *transport_context);
ezo_uart_response_kind_t ezo_uart_device_get_last_response_kind(
    const ezo_uart_device_t *device);
ezo_result_t ezo_uart_send_command(ezo_uart_device_t *device,
                                   const char *command,
                                   ezo_command_kind_t kind,
                                   ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_uart_send_command_with_float(ezo_uart_device_t *device,
                                              const char *prefix,
                                              double value,
                                              uint8_t decimals,
                                              ezo_command_kind_t kind,
                                              ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_uart_send_read(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_uart_send_read_with_temp_comp(ezo_uart_device_t *device,
                                               double temperature_c,
                                               uint8_t decimals,
                                               ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_uart_read_line(ezo_uart_device_t *device,
                                char *buffer,
                                size_t buffer_len,
                                size_t *response_len,
                                ezo_uart_response_kind_t *response_kind);
ezo_result_t ezo_uart_read_terminal_response(ezo_uart_device_t *device,
                                             ezo_uart_response_kind_t *response_kind);
ezo_result_t ezo_uart_read_ok(ezo_uart_device_t *device);
int ezo_uart_response_kind_is_control(ezo_uart_response_kind_t response_kind);
int ezo_uart_response_kind_is_terminal(ezo_uart_response_kind_t response_kind);
ezo_result_t ezo_uart_discard_input(ezo_uart_device_t *device);

typedef enum {
  EZO_PRODUCT_UNKNOWN = 0,
  EZO_PRODUCT_PH,
  EZO_PRODUCT_ORP,
  EZO_PRODUCT_EC,
  EZO_PRODUCT_DO,
  EZO_PRODUCT_RTD,
  EZO_PRODUCT_HUM
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

#define EZO_PRODUCT_SHORT_CODE_MAX_LEN ...
#define EZO_PRODUCT_FIRMWARE_VERSION_MAX_LEN ...
#define EZO_PRODUCT_CAP_MEASUREMENT ...
#define EZO_PRODUCT_CAP_CALIBRATION ...
#define EZO_PRODUCT_CAP_TEMPERATURE_COMPENSATION ...
#define EZO_PRODUCT_CAP_SALINITY_COMPENSATION ...
#define EZO_PRODUCT_CAP_PRESSURE_COMPENSATION ...
#define EZO_PRODUCT_CAP_OUTPUT_SELECTION ...
#define EZO_PRODUCT_CAP_CALIBRATION_TRANSFER ...
#define EZO_PRODUCT_CAP_DATA_LOGGING ...
#define EZO_PRODUCT_CAP_MEMORY_RECALL ...
#define EZO_PRODUCT_CAP_RANGE_SELECTION ...
#define EZO_PRODUCT_CAP_PROBE_CONFIGURATION ...
#define EZO_PRODUCT_CAP_TDS_CONFIGURATION ...
#define EZO_PRODUCT_CAP_SCALE_SELECTION ...
#define EZO_PRODUCT_FAMILY_ACQUISITION ...
#define EZO_PRODUCT_FAMILY_CALIBRATION ...
#define EZO_PRODUCT_FAMILY_CALIBRATION_TRANSFER ...
#define EZO_PRODUCT_FAMILY_IDENTITY ...
#define EZO_PRODUCT_FAMILY_DEVICE_CONTROL ...
#define EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL ...

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

typedef struct {
  const char *text;
  size_t length;
} ezo_text_span_t;

typedef struct {
  size_t line_count;
  size_t data_line_count;
  size_t control_line_count;
  ezo_uart_response_kind_t last_kind;
  ezo_uart_response_kind_t terminal_kind;
  uint8_t complete;
} ezo_uart_sequence_t;

typedef enum {
  EZO_UART_SEQUENCE_STEP_NONE = 0,
  EZO_UART_SEQUENCE_STEP_DATA,
  EZO_UART_SEQUENCE_STEP_CONTROL,
  EZO_UART_SEQUENCE_STEP_TERMINAL
} ezo_uart_sequence_step_t;

int ezo_text_span_is_empty(ezo_text_span_t span);
int ezo_text_span_equals_cstr(ezo_text_span_t span, const char *text);
ezo_result_t ezo_parse_text_span_uint32(ezo_text_span_t span, uint32_t *value_out);
ezo_result_t ezo_parse_text_span_double(ezo_text_span_t span, double *value_out);
ezo_result_t ezo_parse_csv_fields(const char *buffer,
                                  size_t buffer_len,
                                  ezo_text_span_t *fields_out,
                                  size_t fields_capacity,
                                  size_t *field_count_out);
ezo_result_t ezo_parse_query_response(const char *buffer,
                                      size_t buffer_len,
                                      ezo_text_span_t *prefix_out,
                                      ezo_text_span_t *fields_out,
                                      size_t fields_capacity,
                                      size_t *field_count_out);
ezo_result_t ezo_parse_prefixed_fields(const char *buffer,
                                       size_t buffer_len,
                                       const char *prefix,
                                       ezo_text_span_t *fields_out,
                                       size_t fields_capacity,
                                       size_t *field_count_out);
void ezo_uart_sequence_init(ezo_uart_sequence_t *sequence);
ezo_result_t ezo_uart_sequence_push_line(ezo_uart_sequence_t *sequence,
                                         ezo_uart_response_kind_t kind,
                                         ezo_uart_sequence_step_t *step_out);
int ezo_uart_sequence_is_complete(const ezo_uart_sequence_t *sequence);

#define EZO_SCHEMA_MAX_FIELDS ...

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

#define EZO_CONTROL_NAME_MAX_LEN ...

typedef struct {
  char name[EZO_CONTROL_NAME_MAX_LEN];
} ezo_control_name_t;

typedef struct {
  char restart_code;
  double supply_voltage;
} ezo_control_status_t;

typedef struct {
  uint8_t enabled;
} ezo_control_led_status_t;

typedef struct {
  uint8_t enabled;
} ezo_control_protocol_lock_status_t;

typedef struct {
  uint32_t baud_rate;
} ezo_control_baud_status_t;

typedef struct {
  uint8_t enabled;
} ezo_control_response_code_status_t;

ezo_result_t ezo_control_parse_name(const char *buffer,
                                    size_t buffer_len,
                                    ezo_control_name_t *name_out);
ezo_result_t ezo_control_parse_status(const char *buffer,
                                      size_t buffer_len,
                                      ezo_control_status_t *status_out);
ezo_result_t ezo_control_parse_led(const char *buffer,
                                   size_t buffer_len,
                                   ezo_control_led_status_t *status_out);
ezo_result_t ezo_control_parse_protocol_lock(const char *buffer,
                                             size_t buffer_len,
                                             ezo_control_protocol_lock_status_t *status_out);
ezo_result_t ezo_control_parse_baud(const char *buffer,
                                    size_t buffer_len,
                                    ezo_control_baud_status_t *status_out);
ezo_result_t ezo_control_parse_response_code(const char *buffer,
                                             size_t buffer_len,
                                             ezo_control_response_code_status_t *status_out);
ezo_result_t ezo_control_build_name_command(char *buffer, size_t buffer_len, const char *name);
ezo_result_t ezo_control_build_led_command(char *buffer, size_t buffer_len, uint8_t enabled);
ezo_result_t ezo_control_build_protocol_lock_command(char *buffer,
                                                     size_t buffer_len,
                                                     uint8_t enabled);
ezo_result_t ezo_control_build_switch_to_i2c_command(char *buffer,
                                                     size_t buffer_len,
                                                     uint8_t i2c_address);
ezo_result_t ezo_control_build_switch_to_uart_command(char *buffer,
                                                      size_t buffer_len,
                                                      uint32_t baud_rate);
ezo_result_t ezo_control_build_response_code_command(char *buffer,
                                                     size_t buffer_len,
                                                     uint8_t enabled);
ezo_result_t ezo_control_send_info_query_i2c(ezo_i2c_device_t *device,
                                             ezo_product_id_t product_id,
                                             ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_name_query_i2c(ezo_i2c_device_t *device,
                                             ezo_product_id_t product_id,
                                             ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_name_set_i2c(ezo_i2c_device_t *device,
                                           ezo_product_id_t product_id,
                                           const char *name,
                                           ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_status_query_i2c(ezo_i2c_device_t *device,
                                               ezo_product_id_t product_id,
                                               ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_led_query_i2c(ezo_i2c_device_t *device,
                                            ezo_product_id_t product_id,
                                            ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_led_set_i2c(ezo_i2c_device_t *device,
                                          ezo_product_id_t product_id,
                                          uint8_t enabled,
                                          ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_find_i2c(ezo_i2c_device_t *device,
                                       ezo_product_id_t product_id,
                                       ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_sleep_i2c(ezo_i2c_device_t *device,
                                        ezo_product_id_t product_id,
                                        ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_factory_reset_i2c(ezo_i2c_device_t *device,
                                                ezo_product_id_t product_id,
                                                ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_protocol_lock_query_i2c(ezo_i2c_device_t *device,
                                                      ezo_product_id_t product_id,
                                                      ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_protocol_lock_set_i2c(ezo_i2c_device_t *device,
                                                    ezo_product_id_t product_id,
                                                    uint8_t enabled,
                                                    ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_switch_to_uart_i2c(ezo_i2c_device_t *device,
                                                 ezo_product_id_t product_id,
                                                 uint32_t baud_rate,
                                                 ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_read_info_i2c(ezo_i2c_device_t *device, ezo_device_info_t *info_out);
ezo_result_t ezo_control_read_name_i2c(ezo_i2c_device_t *device, ezo_control_name_t *name_out);
ezo_result_t ezo_control_read_status_i2c(ezo_i2c_device_t *device,
                                         ezo_control_status_t *status_out);
ezo_result_t ezo_control_read_led_i2c(ezo_i2c_device_t *device,
                                      ezo_control_led_status_t *status_out);
ezo_result_t ezo_control_read_protocol_lock_i2c(
    ezo_i2c_device_t *device,
    ezo_control_protocol_lock_status_t *status_out);
ezo_result_t ezo_control_send_info_query_uart(ezo_uart_device_t *device,
                                              ezo_product_id_t product_id,
                                              ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_name_query_uart(ezo_uart_device_t *device,
                                              ezo_product_id_t product_id,
                                              ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_name_set_uart(ezo_uart_device_t *device,
                                            ezo_product_id_t product_id,
                                            const char *name,
                                            ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_status_query_uart(ezo_uart_device_t *device,
                                                ezo_product_id_t product_id,
                                                ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_led_query_uart(ezo_uart_device_t *device,
                                             ezo_product_id_t product_id,
                                             ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_led_set_uart(ezo_uart_device_t *device,
                                           ezo_product_id_t product_id,
                                           uint8_t enabled,
                                           ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_find_uart(ezo_uart_device_t *device,
                                        ezo_product_id_t product_id,
                                        ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_sleep_uart(ezo_uart_device_t *device,
                                         ezo_product_id_t product_id,
                                         ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_factory_reset_uart(ezo_uart_device_t *device,
                                                 ezo_product_id_t product_id,
                                                 ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_protocol_lock_query_uart(
    ezo_uart_device_t *device,
    ezo_product_id_t product_id,
    ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_protocol_lock_set_uart(
    ezo_uart_device_t *device,
    ezo_product_id_t product_id,
    uint8_t enabled,
    ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_baud_query_uart(ezo_uart_device_t *device,
                                              ezo_product_id_t product_id,
                                              ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_response_code_query_uart(ezo_uart_device_t *device,
                                                       ezo_product_id_t product_id,
                                                       ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_response_code_set_uart(ezo_uart_device_t *device,
                                                     ezo_product_id_t product_id,
                                                     uint8_t enabled,
                                                     ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_switch_to_i2c_uart(ezo_uart_device_t *device,
                                                 ezo_product_id_t product_id,
                                                 uint8_t i2c_address,
                                                 ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_send_switch_to_uart_uart(ezo_uart_device_t *device,
                                                  ezo_product_id_t product_id,
                                                  uint32_t baud_rate,
                                                  ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_control_read_info_uart(ezo_uart_device_t *device, ezo_device_info_t *info_out);
ezo_result_t ezo_control_read_name_uart(ezo_uart_device_t *device,
                                        ezo_control_name_t *name_out);
ezo_result_t ezo_control_read_status_uart(ezo_uart_device_t *device,
                                          ezo_control_status_t *status_out);
ezo_result_t ezo_control_read_led_uart(ezo_uart_device_t *device,
                                       ezo_control_led_status_t *status_out);
ezo_result_t ezo_control_read_protocol_lock_uart(
    ezo_uart_device_t *device,
    ezo_control_protocol_lock_status_t *status_out);
ezo_result_t ezo_control_read_baud_uart(ezo_uart_device_t *device,
                                        ezo_control_baud_status_t *status_out);
ezo_result_t ezo_control_read_response_code_uart(
    ezo_uart_device_t *device,
    ezo_control_response_code_status_t *status_out);

typedef struct {
  uint32_t chunk_count;
  uint32_t byte_count;
} ezo_calibration_export_info_t;

typedef struct {
  ezo_device_status_t device_status;
  uint8_t pending_reboot;
} ezo_calibration_import_result_t;

ezo_result_t ezo_calibration_parse_export_info(const char *buffer,
                                               size_t buffer_len,
                                               ezo_calibration_export_info_t *info_out);
ezo_result_t ezo_calibration_build_import_command(char *buffer,
                                                  size_t buffer_len,
                                                  const char *payload);
ezo_result_t ezo_calibration_send_export_info_query_i2c(ezo_i2c_device_t *device,
                                                        ezo_product_id_t product_id,
                                                        ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_calibration_send_export_next_i2c(ezo_i2c_device_t *device,
                                                  ezo_product_id_t product_id,
                                                  ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_calibration_send_import_i2c(ezo_i2c_device_t *device,
                                             ezo_product_id_t product_id,
                                             const char *payload,
                                             ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_calibration_read_export_info_i2c(ezo_i2c_device_t *device,
                                                  ezo_calibration_export_info_t *info_out);
ezo_result_t ezo_calibration_read_export_chunk_i2c(ezo_i2c_device_t *device,
                                                   char *buffer,
                                                   size_t buffer_len,
                                                   size_t *chunk_len_out);
ezo_result_t ezo_calibration_read_import_status_i2c(ezo_i2c_device_t *device,
                                                    ezo_device_status_t *status_out);
ezo_result_t ezo_calibration_read_import_result_i2c(
    ezo_i2c_device_t *device,
    ezo_calibration_import_result_t *result_out);
ezo_result_t ezo_calibration_send_export_info_query_uart(ezo_uart_device_t *device,
                                                         ezo_product_id_t product_id,
                                                         ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_calibration_send_export_next_uart(ezo_uart_device_t *device,
                                                   ezo_product_id_t product_id,
                                                   ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_calibration_send_import_uart(ezo_uart_device_t *device,
                                              ezo_product_id_t product_id,
                                              const char *payload,
                                              ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_calibration_read_export_info_uart(ezo_uart_device_t *device,
                                                   ezo_calibration_export_info_t *info_out);
ezo_result_t ezo_calibration_read_export_chunk_uart(ezo_uart_device_t *device,
                                                    char *buffer,
                                                    size_t buffer_len,
                                                    size_t *chunk_len_out,
                                                    ezo_uart_response_kind_t *response_kind_out);
ezo_result_t ezo_calibration_read_import_result_uart(ezo_uart_device_t *device,
                                                     ezo_uart_response_kind_t *response_kind_out);

typedef enum {
  EZO_PH_CALIBRATION_NONE = 0,
  EZO_PH_CALIBRATION_ONE_POINT = 1,
  EZO_PH_CALIBRATION_TWO_POINT = 2,
  EZO_PH_CALIBRATION_THREE_POINT = 3
} ezo_ph_calibration_level_t;

typedef enum {
  EZO_PH_CALIBRATION_POINT_MID = 0,
  EZO_PH_CALIBRATION_POINT_LOW,
  EZO_PH_CALIBRATION_POINT_HIGH
} ezo_ph_calibration_point_t;

typedef enum {
  EZO_PH_EXTENDED_RANGE_DISABLED = 0,
  EZO_PH_EXTENDED_RANGE_ENABLED = 1
} ezo_ph_extended_range_t;

typedef struct { double ph; } ezo_ph_reading_t;
typedef struct { double temperature_c; } ezo_ph_temperature_compensation_t;
typedef struct { ezo_ph_calibration_level_t level; } ezo_ph_calibration_status_t;
typedef struct {
  double acid_percent;
  double base_percent;
  double neutral_mv;
} ezo_ph_slope_t;
typedef struct { ezo_ph_extended_range_t enabled; } ezo_ph_extended_range_status_t;

ezo_result_t ezo_ph_parse_reading(const char *buffer, size_t buffer_len, ezo_ph_reading_t *reading_out);
ezo_result_t ezo_ph_parse_temperature(const char *buffer, size_t buffer_len, ezo_ph_temperature_compensation_t *temperature_out);
ezo_result_t ezo_ph_parse_calibration_status(const char *buffer, size_t buffer_len, ezo_ph_calibration_status_t *status_out);
ezo_result_t ezo_ph_parse_slope(const char *buffer, size_t buffer_len, ezo_ph_slope_t *slope_out);
ezo_result_t ezo_ph_parse_extended_range(const char *buffer, size_t buffer_len, ezo_ph_extended_range_status_t *status_out);
ezo_result_t ezo_ph_build_temperature_command(char *buffer, size_t buffer_len, double temperature_c, uint8_t decimals);
ezo_result_t ezo_ph_build_calibration_command(char *buffer, size_t buffer_len, ezo_ph_calibration_point_t point, double reference_ph, uint8_t decimals);
ezo_result_t ezo_ph_build_extended_range_command(char *buffer, size_t buffer_len, ezo_ph_extended_range_t enabled);
ezo_result_t ezo_ph_send_read_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_temperature_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_temperature_set_i2c(ezo_i2c_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_calibration_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_calibration_i2c(ezo_i2c_device_t *device, ezo_ph_calibration_point_t point, double reference_ph, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_clear_calibration_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_slope_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_extended_range_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_extended_range_set_i2c(ezo_i2c_device_t *device, ezo_ph_extended_range_t enabled, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_read_response_i2c(ezo_i2c_device_t *device, ezo_ph_reading_t *reading_out);
ezo_result_t ezo_ph_read_temperature_i2c(ezo_i2c_device_t *device, ezo_ph_temperature_compensation_t *temperature_out);
ezo_result_t ezo_ph_read_calibration_status_i2c(ezo_i2c_device_t *device, ezo_ph_calibration_status_t *status_out);
ezo_result_t ezo_ph_read_slope_i2c(ezo_i2c_device_t *device, ezo_ph_slope_t *slope_out);
ezo_result_t ezo_ph_read_extended_range_i2c(ezo_i2c_device_t *device, ezo_ph_extended_range_status_t *status_out);
ezo_result_t ezo_ph_send_read_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_read_with_temp_comp_uart(ezo_uart_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_temperature_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_temperature_set_uart(ezo_uart_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_calibration_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_calibration_uart(ezo_uart_device_t *device, ezo_ph_calibration_point_t point, double reference_ph, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_clear_calibration_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_slope_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_extended_range_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_send_extended_range_set_uart(ezo_uart_device_t *device, ezo_ph_extended_range_t enabled, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ph_read_response_uart(ezo_uart_device_t *device, ezo_ph_reading_t *reading_out);
ezo_result_t ezo_ph_read_response_with_temp_comp_uart(ezo_uart_device_t *device, ezo_ph_reading_t *reading_out);
ezo_result_t ezo_ph_read_temperature_uart(ezo_uart_device_t *device, ezo_ph_temperature_compensation_t *temperature_out);
ezo_result_t ezo_ph_read_calibration_status_uart(ezo_uart_device_t *device, ezo_ph_calibration_status_t *status_out);
ezo_result_t ezo_ph_read_slope_uart(ezo_uart_device_t *device, ezo_ph_slope_t *slope_out);
ezo_result_t ezo_ph_read_extended_range_uart(ezo_uart_device_t *device, ezo_ph_extended_range_status_t *status_out);

typedef enum {
  EZO_ORP_EXTENDED_SCALE_DISABLED = 0,
  EZO_ORP_EXTENDED_SCALE_ENABLED = 1
} ezo_orp_extended_scale_t;

typedef struct { double millivolts; } ezo_orp_reading_t;
typedef struct { uint8_t calibrated; } ezo_orp_calibration_status_t;
typedef struct { ezo_orp_extended_scale_t enabled; } ezo_orp_extended_scale_status_t;

ezo_result_t ezo_orp_parse_reading(const char *buffer, size_t buffer_len, ezo_orp_reading_t *reading_out);
ezo_result_t ezo_orp_parse_calibration_status(const char *buffer, size_t buffer_len, ezo_orp_calibration_status_t *status_out);
ezo_result_t ezo_orp_parse_extended_scale(const char *buffer, size_t buffer_len, ezo_orp_extended_scale_status_t *status_out);
ezo_result_t ezo_orp_build_calibration_command(char *buffer, size_t buffer_len, double reference_mv, uint8_t decimals);
ezo_result_t ezo_orp_build_extended_scale_command(char *buffer, size_t buffer_len, ezo_orp_extended_scale_t extended_scale);
ezo_result_t ezo_orp_send_read_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_calibration_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_calibration_i2c(ezo_i2c_device_t *device, double reference_mv, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_clear_calibration_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_extended_scale_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_extended_scale_set_i2c(ezo_i2c_device_t *device, ezo_orp_extended_scale_t extended_scale, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_read_response_i2c(ezo_i2c_device_t *device, ezo_orp_reading_t *reading_out);
ezo_result_t ezo_orp_read_calibration_status_i2c(ezo_i2c_device_t *device, ezo_orp_calibration_status_t *status_out);
ezo_result_t ezo_orp_read_extended_scale_i2c(ezo_i2c_device_t *device, ezo_orp_extended_scale_status_t *status_out);
ezo_result_t ezo_orp_send_read_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_calibration_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_calibration_uart(ezo_uart_device_t *device, double reference_mv, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_clear_calibration_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_extended_scale_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_send_extended_scale_set_uart(ezo_uart_device_t *device, ezo_orp_extended_scale_t extended_scale, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_orp_read_response_uart(ezo_uart_device_t *device, ezo_orp_reading_t *reading_out);
ezo_result_t ezo_orp_read_calibration_status_uart(ezo_uart_device_t *device, ezo_orp_calibration_status_t *status_out);
ezo_result_t ezo_orp_read_extended_scale_uart(ezo_uart_device_t *device, ezo_orp_extended_scale_status_t *status_out);

typedef enum {
  EZO_RTD_SCALE_UNKNOWN = 0,
  EZO_RTD_SCALE_CELSIUS,
  EZO_RTD_SCALE_KELVIN,
  EZO_RTD_SCALE_FAHRENHEIT
} ezo_rtd_scale_t;

typedef struct { double temperature; ezo_rtd_scale_t scale; } ezo_rtd_reading_t;
typedef struct { ezo_rtd_scale_t scale; } ezo_rtd_scale_status_t;
typedef struct { uint8_t calibrated; } ezo_rtd_calibration_status_t;
typedef struct { uint32_t interval_units; } ezo_rtd_logger_status_t;
typedef struct { uint32_t last_index; } ezo_rtd_memory_status_t;
typedef struct { uint32_t index; double temperature; ezo_rtd_scale_t scale; } ezo_rtd_memory_entry_t;
typedef struct { double temperature; ezo_rtd_scale_t scale; } ezo_rtd_memory_value_t;

ezo_result_t ezo_rtd_parse_reading(const char *buffer, size_t buffer_len, ezo_rtd_scale_t scale, ezo_rtd_reading_t *reading_out);
ezo_result_t ezo_rtd_parse_scale(const char *buffer, size_t buffer_len, ezo_rtd_scale_status_t *status_out);
ezo_result_t ezo_rtd_parse_calibration_status(const char *buffer, size_t buffer_len, ezo_rtd_calibration_status_t *status_out);
ezo_result_t ezo_rtd_parse_logger_status(const char *buffer, size_t buffer_len, ezo_rtd_logger_status_t *status_out);
ezo_result_t ezo_rtd_parse_memory_status(const char *buffer, size_t buffer_len, ezo_rtd_memory_status_t *status_out);
ezo_result_t ezo_rtd_parse_memory_entry(const char *buffer, size_t buffer_len, ezo_rtd_scale_t scale, ezo_rtd_memory_entry_t *entry_out);
ezo_result_t ezo_rtd_parse_memory_all(const char *buffer, size_t buffer_len, ezo_rtd_scale_t scale, ezo_rtd_memory_value_t *values_out, size_t values_capacity, size_t *value_count_out);
ezo_result_t ezo_rtd_build_scale_command(char *buffer, size_t buffer_len, ezo_rtd_scale_t scale);
ezo_result_t ezo_rtd_build_calibration_command(char *buffer, size_t buffer_len, double reference_temperature, uint8_t decimals);
ezo_result_t ezo_rtd_build_logger_command(char *buffer, size_t buffer_len, uint32_t interval_units);
ezo_result_t ezo_rtd_send_read_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_scale_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_scale_set_i2c(ezo_i2c_device_t *device, ezo_rtd_scale_t scale, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_calibration_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_calibration_i2c(ezo_i2c_device_t *device, double reference_temperature, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_clear_calibration_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_logger_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_logger_set_i2c(ezo_i2c_device_t *device, uint32_t interval_units, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_memory_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_memory_next_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_memory_all_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_memory_clear_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_read_response_i2c(ezo_i2c_device_t *device, ezo_rtd_scale_t scale, ezo_rtd_reading_t *reading_out);
ezo_result_t ezo_rtd_read_scale_i2c(ezo_i2c_device_t *device, ezo_rtd_scale_status_t *status_out);
ezo_result_t ezo_rtd_read_calibration_status_i2c(ezo_i2c_device_t *device, ezo_rtd_calibration_status_t *status_out);
ezo_result_t ezo_rtd_read_logger_i2c(ezo_i2c_device_t *device, ezo_rtd_logger_status_t *status_out);
ezo_result_t ezo_rtd_read_memory_status_i2c(ezo_i2c_device_t *device, ezo_rtd_memory_status_t *status_out);
ezo_result_t ezo_rtd_read_memory_entry_i2c(ezo_i2c_device_t *device, ezo_rtd_scale_t scale, ezo_rtd_memory_entry_t *entry_out);
ezo_result_t ezo_rtd_read_memory_all_i2c(ezo_i2c_device_t *device, ezo_rtd_scale_t scale, ezo_rtd_memory_value_t *values_out, size_t values_capacity, size_t *value_count_out);
ezo_result_t ezo_rtd_send_read_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_scale_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_scale_set_uart(ezo_uart_device_t *device, ezo_rtd_scale_t scale, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_calibration_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_calibration_uart(ezo_uart_device_t *device, double reference_temperature, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_clear_calibration_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_logger_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_logger_set_uart(ezo_uart_device_t *device, uint32_t interval_units, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_memory_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_memory_next_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_memory_all_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_send_memory_clear_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_rtd_read_response_uart(ezo_uart_device_t *device, ezo_rtd_scale_t scale, ezo_rtd_reading_t *reading_out);
ezo_result_t ezo_rtd_read_scale_uart(ezo_uart_device_t *device, ezo_rtd_scale_status_t *status_out);
ezo_result_t ezo_rtd_read_calibration_status_uart(ezo_uart_device_t *device, ezo_rtd_calibration_status_t *status_out);
ezo_result_t ezo_rtd_read_logger_uart(ezo_uart_device_t *device, ezo_rtd_logger_status_t *status_out);
ezo_result_t ezo_rtd_read_memory_status_uart(ezo_uart_device_t *device, ezo_rtd_memory_status_t *status_out);
ezo_result_t ezo_rtd_read_memory_entry_uart(ezo_uart_device_t *device, ezo_rtd_scale_t scale, ezo_rtd_memory_entry_t *entry_out);
ezo_result_t ezo_rtd_read_memory_all_uart(ezo_uart_device_t *device, ezo_rtd_scale_t scale, ezo_rtd_memory_value_t *values_out, size_t values_capacity, size_t *value_count_out);

typedef uint32_t ezo_ec_output_mask_t;
#define EZO_EC_OUTPUT_CONDUCTIVITY ...
#define EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS ...
#define EZO_EC_OUTPUT_SALINITY ...
#define EZO_EC_OUTPUT_SPECIFIC_GRAVITY ...

typedef struct {
  ezo_ec_output_mask_t present_mask;
  double conductivity_us_cm;
  double total_dissolved_solids_ppm;
  double salinity_ppt;
  double specific_gravity;
} ezo_ec_reading_t;
typedef struct { ezo_ec_output_mask_t enabled_mask; } ezo_ec_output_config_t;
typedef struct { double temperature_c; } ezo_ec_temperature_compensation_t;
typedef struct { double k_value; } ezo_ec_probe_k_t;
typedef struct { double factor; } ezo_ec_tds_factor_t;
typedef enum {
  EZO_EC_CALIBRATION_DRY = 0,
  EZO_EC_CALIBRATION_SINGLE_POINT,
  EZO_EC_CALIBRATION_LOW_POINT,
  EZO_EC_CALIBRATION_HIGH_POINT
} ezo_ec_calibration_point_t;
typedef struct { uint32_t level; } ezo_ec_calibration_status_t;

ezo_result_t ezo_ec_parse_reading(const char *buffer, size_t buffer_len, ezo_ec_output_mask_t enabled_mask, ezo_ec_reading_t *reading_out);
ezo_result_t ezo_ec_parse_output_config(const char *buffer, size_t buffer_len, ezo_ec_output_config_t *config_out);
ezo_result_t ezo_ec_parse_temperature(const char *buffer, size_t buffer_len, ezo_ec_temperature_compensation_t *temperature_out);
ezo_result_t ezo_ec_parse_probe_k(const char *buffer, size_t buffer_len, ezo_ec_probe_k_t *probe_k_out);
ezo_result_t ezo_ec_parse_tds_factor(const char *buffer, size_t buffer_len, ezo_ec_tds_factor_t *tds_factor_out);
ezo_result_t ezo_ec_parse_calibration_status(const char *buffer, size_t buffer_len, ezo_ec_calibration_status_t *status_out);
ezo_result_t ezo_ec_build_output_command(char *buffer, size_t buffer_len, ezo_ec_output_mask_t output, uint8_t enabled);
ezo_result_t ezo_ec_build_temperature_command(char *buffer, size_t buffer_len, double temperature_c, uint8_t decimals);
ezo_result_t ezo_ec_build_probe_k_command(char *buffer, size_t buffer_len, double k_value, uint8_t decimals);
ezo_result_t ezo_ec_build_tds_factor_command(char *buffer, size_t buffer_len, double factor, uint8_t decimals);
ezo_result_t ezo_ec_build_calibration_command(char *buffer, size_t buffer_len, ezo_ec_calibration_point_t point, double reference_value, uint8_t decimals);
ezo_result_t ezo_ec_send_read_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_output_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_output_set_i2c(ezo_i2c_device_t *device, ezo_ec_output_mask_t output, uint8_t enabled, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_temperature_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_temperature_set_i2c(ezo_i2c_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_probe_k_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_probe_k_set_i2c(ezo_i2c_device_t *device, double k_value, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_tds_factor_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_tds_factor_set_i2c(ezo_i2c_device_t *device, double factor, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_calibration_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_calibration_i2c(ezo_i2c_device_t *device, ezo_ec_calibration_point_t point, double reference_value, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_clear_calibration_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_read_response_i2c(ezo_i2c_device_t *device, ezo_ec_output_mask_t enabled_mask, ezo_ec_reading_t *reading_out);
ezo_result_t ezo_ec_read_output_config_i2c(ezo_i2c_device_t *device, ezo_ec_output_config_t *config_out);
ezo_result_t ezo_ec_read_temperature_i2c(ezo_i2c_device_t *device, ezo_ec_temperature_compensation_t *temperature_out);
ezo_result_t ezo_ec_read_probe_k_i2c(ezo_i2c_device_t *device, ezo_ec_probe_k_t *probe_k_out);
ezo_result_t ezo_ec_read_tds_factor_i2c(ezo_i2c_device_t *device, ezo_ec_tds_factor_t *tds_factor_out);
ezo_result_t ezo_ec_read_calibration_status_i2c(ezo_i2c_device_t *device, ezo_ec_calibration_status_t *status_out);
ezo_result_t ezo_ec_send_read_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_read_with_temp_comp_uart(ezo_uart_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_output_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_output_set_uart(ezo_uart_device_t *device, ezo_ec_output_mask_t output, uint8_t enabled, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_temperature_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_temperature_set_uart(ezo_uart_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_probe_k_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_probe_k_set_uart(ezo_uart_device_t *device, double k_value, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_tds_factor_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_tds_factor_set_uart(ezo_uart_device_t *device, double factor, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_calibration_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_calibration_uart(ezo_uart_device_t *device, ezo_ec_calibration_point_t point, double reference_value, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_send_clear_calibration_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_ec_read_response_uart(ezo_uart_device_t *device, ezo_ec_output_mask_t enabled_mask, ezo_ec_reading_t *reading_out);
ezo_result_t ezo_ec_read_output_config_uart(ezo_uart_device_t *device, ezo_ec_output_config_t *config_out);
ezo_result_t ezo_ec_read_temperature_uart(ezo_uart_device_t *device, ezo_ec_temperature_compensation_t *temperature_out);
ezo_result_t ezo_ec_read_probe_k_uart(ezo_uart_device_t *device, ezo_ec_probe_k_t *probe_k_out);
ezo_result_t ezo_ec_read_tds_factor_uart(ezo_uart_device_t *device, ezo_ec_tds_factor_t *tds_factor_out);
ezo_result_t ezo_ec_read_calibration_status_uart(ezo_uart_device_t *device, ezo_ec_calibration_status_t *status_out);

typedef uint32_t ezo_do_output_mask_t;
#define EZO_DO_OUTPUT_MG_L ...
#define EZO_DO_OUTPUT_PERCENT_SATURATION ...

typedef enum {
  EZO_DO_SALINITY_UNIT_MICROSIEMENS = 0,
  EZO_DO_SALINITY_UNIT_PPT
} ezo_do_salinity_unit_t;

typedef enum {
  EZO_DO_CALIBRATION_ATMOSPHERIC = 0,
  EZO_DO_CALIBRATION_ZERO
} ezo_do_calibration_point_t;

typedef struct {
  ezo_do_output_mask_t present_mask;
  double milligrams_per_liter;
  double percent_saturation;
} ezo_do_reading_t;
typedef struct { ezo_do_output_mask_t enabled_mask; } ezo_do_output_config_t;
typedef struct { double temperature_c; } ezo_do_temperature_compensation_t;
typedef struct { double value; ezo_do_salinity_unit_t unit; } ezo_do_salinity_compensation_t;
typedef struct { double pressure_kpa; } ezo_do_pressure_compensation_t;
typedef struct { uint32_t level; } ezo_do_calibration_status_t;

ezo_result_t ezo_do_parse_reading(const char *buffer, size_t buffer_len, ezo_do_output_mask_t enabled_mask, ezo_do_reading_t *reading_out);
ezo_result_t ezo_do_parse_output_config(const char *buffer, size_t buffer_len, ezo_do_output_config_t *config_out);
ezo_result_t ezo_do_parse_temperature(const char *buffer, size_t buffer_len, ezo_do_temperature_compensation_t *temperature_out);
ezo_result_t ezo_do_parse_salinity(const char *buffer, size_t buffer_len, ezo_do_salinity_compensation_t *salinity_out);
ezo_result_t ezo_do_parse_pressure(const char *buffer, size_t buffer_len, ezo_do_pressure_compensation_t *pressure_out);
ezo_result_t ezo_do_parse_calibration_status(const char *buffer, size_t buffer_len, ezo_do_calibration_status_t *status_out);
ezo_result_t ezo_do_build_output_command(char *buffer, size_t buffer_len, ezo_do_output_mask_t output, uint8_t enabled);
ezo_result_t ezo_do_build_temperature_command(char *buffer, size_t buffer_len, double temperature_c, uint8_t decimals);
ezo_result_t ezo_do_build_salinity_command(char *buffer, size_t buffer_len, double value, ezo_do_salinity_unit_t unit, uint8_t decimals);
ezo_result_t ezo_do_build_pressure_command(char *buffer, size_t buffer_len, double pressure_kpa, uint8_t decimals);
ezo_result_t ezo_do_build_calibration_command(char *buffer, size_t buffer_len, ezo_do_calibration_point_t point);
ezo_result_t ezo_do_send_read_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_output_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_output_set_i2c(ezo_i2c_device_t *device, ezo_do_output_mask_t output, uint8_t enabled, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_temperature_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_temperature_set_i2c(ezo_i2c_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_salinity_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_salinity_set_i2c(ezo_i2c_device_t *device, double value, ezo_do_salinity_unit_t unit, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_pressure_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_pressure_set_i2c(ezo_i2c_device_t *device, double pressure_kpa, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_calibration_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_calibration_i2c(ezo_i2c_device_t *device, ezo_do_calibration_point_t point, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_clear_calibration_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_read_response_i2c(ezo_i2c_device_t *device, ezo_do_output_mask_t enabled_mask, ezo_do_reading_t *reading_out);
ezo_result_t ezo_do_read_output_config_i2c(ezo_i2c_device_t *device, ezo_do_output_config_t *config_out);
ezo_result_t ezo_do_read_temperature_i2c(ezo_i2c_device_t *device, ezo_do_temperature_compensation_t *temperature_out);
ezo_result_t ezo_do_read_salinity_i2c(ezo_i2c_device_t *device, ezo_do_salinity_compensation_t *salinity_out);
ezo_result_t ezo_do_read_pressure_i2c(ezo_i2c_device_t *device, ezo_do_pressure_compensation_t *pressure_out);
ezo_result_t ezo_do_read_calibration_status_i2c(ezo_i2c_device_t *device, ezo_do_calibration_status_t *status_out);
ezo_result_t ezo_do_send_read_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_read_with_temp_comp_uart(ezo_uart_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_output_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_output_set_uart(ezo_uart_device_t *device, ezo_do_output_mask_t output, uint8_t enabled, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_temperature_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_temperature_set_uart(ezo_uart_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_salinity_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_salinity_set_uart(ezo_uart_device_t *device, double value, ezo_do_salinity_unit_t unit, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_pressure_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_pressure_set_uart(ezo_uart_device_t *device, double pressure_kpa, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_calibration_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_calibration_uart(ezo_uart_device_t *device, ezo_do_calibration_point_t point, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_send_clear_calibration_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_do_read_response_uart(ezo_uart_device_t *device, ezo_do_output_mask_t enabled_mask, ezo_do_reading_t *reading_out);
ezo_result_t ezo_do_read_output_config_uart(ezo_uart_device_t *device, ezo_do_output_config_t *config_out);
ezo_result_t ezo_do_read_temperature_uart(ezo_uart_device_t *device, ezo_do_temperature_compensation_t *temperature_out);
ezo_result_t ezo_do_read_salinity_uart(ezo_uart_device_t *device, ezo_do_salinity_compensation_t *salinity_out);
ezo_result_t ezo_do_read_pressure_uart(ezo_uart_device_t *device, ezo_do_pressure_compensation_t *pressure_out);
ezo_result_t ezo_do_read_calibration_status_uart(ezo_uart_device_t *device, ezo_do_calibration_status_t *status_out);

typedef uint32_t ezo_hum_output_mask_t;
#define EZO_HUM_OUTPUT_HUMIDITY ...
#define EZO_HUM_OUTPUT_AIR_TEMPERATURE ...
#define EZO_HUM_OUTPUT_DEW_POINT ...

typedef struct {
  ezo_hum_output_mask_t present_mask;
  double relative_humidity_percent;
  double air_temperature_c;
  double dew_point_c;
} ezo_hum_reading_t;
typedef struct { ezo_hum_output_mask_t enabled_mask; } ezo_hum_output_config_t;
typedef struct { uint8_t calibrated; } ezo_hum_temperature_calibration_status_t;

ezo_result_t ezo_hum_parse_reading(const char *buffer, size_t buffer_len, ezo_hum_output_mask_t enabled_mask, ezo_hum_reading_t *reading_out);
ezo_result_t ezo_hum_parse_output_config(const char *buffer, size_t buffer_len, ezo_hum_output_config_t *config_out);
ezo_result_t ezo_hum_parse_temperature_calibration_status(const char *buffer, size_t buffer_len, ezo_hum_temperature_calibration_status_t *status_out);
ezo_result_t ezo_hum_build_output_command(char *buffer, size_t buffer_len, ezo_hum_output_mask_t output, uint8_t enabled);
ezo_result_t ezo_hum_build_temperature_calibration_command(char *buffer, size_t buffer_len, double temperature_c, uint8_t decimals);
ezo_result_t ezo_hum_send_read_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_output_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_output_set_i2c(ezo_i2c_device_t *device, ezo_hum_output_mask_t output, uint8_t enabled, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_temperature_calibration_query_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_temperature_calibration_i2c(ezo_i2c_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_clear_temperature_calibration_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_read_response_i2c(ezo_i2c_device_t *device, ezo_hum_output_mask_t enabled_mask, ezo_hum_reading_t *reading_out);
ezo_result_t ezo_hum_read_output_config_i2c(ezo_i2c_device_t *device, ezo_hum_output_config_t *config_out);
ezo_result_t ezo_hum_read_temperature_calibration_status_i2c(ezo_i2c_device_t *device, ezo_hum_temperature_calibration_status_t *status_out);
ezo_result_t ezo_hum_send_read_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_output_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_output_set_uart(ezo_uart_device_t *device, ezo_hum_output_mask_t output, uint8_t enabled, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_temperature_calibration_query_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_temperature_calibration_uart(ezo_uart_device_t *device, double temperature_c, uint8_t decimals, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_send_clear_temperature_calibration_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);
ezo_result_t ezo_hum_read_response_uart(ezo_uart_device_t *device, ezo_hum_output_mask_t enabled_mask, ezo_hum_reading_t *reading_out);
ezo_result_t ezo_hum_read_output_config_uart(ezo_uart_device_t *device, ezo_hum_output_config_t *config_out);
ezo_result_t ezo_hum_read_temperature_calibration_status_uart(ezo_uart_device_t *device, ezo_hum_temperature_calibration_status_t *status_out);

typedef struct { int fd; } ezo_linux_i2c_context_t;

typedef struct {
  int fd;
  uint32_t read_timeout_ms;
  int owns_fd;
  int has_saved_termios;
  struct termios saved_termios;
} ezo_uart_posix_serial_t;

typedef struct {
  ezo_i2c_device_t core;
  ezo_linux_i2c_context_t context;
  int owns_fd;
} ezo_linux_i2c_device_t;

typedef struct {
  ezo_uart_device_t core;
  ezo_uart_posix_serial_t serial;
} ezo_linux_uart_device_t;

ezo_result_t ezo_linux_i2c_device_open_bus(ezo_linux_i2c_device_t *device,
                                           uint32_t bus_index,
                                           uint8_t address);
ezo_result_t ezo_linux_i2c_device_open_path(ezo_linux_i2c_device_t *device,
                                            const char *path,
                                            uint8_t address);
void ezo_linux_i2c_device_close(ezo_linux_i2c_device_t *device);
ezo_i2c_device_t *ezo_linux_i2c_device_core(ezo_linux_i2c_device_t *device);
ezo_result_t ezo_linux_uart_device_open(ezo_linux_uart_device_t *device,
                                        const char *path,
                                        uint32_t baud_rate,
                                        uint32_t read_timeout_ms);
void ezo_linux_uart_device_close(ezo_linux_uart_device_t *device);
ezo_uart_device_t *ezo_linux_uart_device_core(ezo_linux_uart_device_t *device);
