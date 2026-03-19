#ifndef EZO_I2C_EZO_I2C_H
#define EZO_I2C_EZO_I2C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EZO_I2C_MAX_TEXT_RESPONSE_LEN 255

typedef enum {
  EZO_OK = 0,
  EZO_ERR_INVALID_ARGUMENT,
  EZO_ERR_BUFFER_TOO_SMALL,
  EZO_ERR_TRANSPORT,
  EZO_ERR_PROTOCOL,
  EZO_ERR_PARSE,
  EZO_ERR_STATE
} ezo_result_t;

typedef enum {
  EZO_STATUS_UNKNOWN = 0,
  EZO_STATUS_SUCCESS,
  EZO_STATUS_FAIL,
  EZO_STATUS_NOT_READY,
  EZO_STATUS_NO_DATA
} ezo_device_status_t;

typedef enum {
  EZO_COMMAND_GENERIC = 0,
  EZO_COMMAND_READ,
  EZO_COMMAND_READ_WITH_TEMP_COMP,
  EZO_COMMAND_CALIBRATION
} ezo_command_kind_t;

typedef struct {
  uint32_t wait_ms;
} ezo_timing_hint_t;

typedef struct ezo_i2c_transport {
  ezo_result_t (*write_then_read)(void *context,
                                  uint8_t address,
                                  const uint8_t *tx_data,
                                  size_t tx_len,
                                  uint8_t *rx_data,
                                  size_t rx_len,
                                  size_t *rx_received);
} ezo_i2c_transport_t;

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

ezo_result_t ezo_get_timing_hint_for_command_kind(ezo_command_kind_t kind,
                                                  ezo_timing_hint_t *timing_hint);

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

ezo_result_t ezo_read_response(ezo_i2c_device_t *device,
                               char *buffer,
                               size_t buffer_len,
                               size_t *response_len,
                               ezo_device_status_t *device_status);

ezo_result_t ezo_parse_double(const char *buffer, size_t buffer_len, double *value_out);

#ifdef __cplusplus
}
#endif

#endif
