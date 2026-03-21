#ifndef EZO_HUM_H
#define EZO_HUM_H

#include "ezo_i2c.h"
#include "ezo_uart.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ezo_hum_output_mask_t;

enum {
  EZO_HUM_OUTPUT_HUMIDITY = 1u << 0,
  EZO_HUM_OUTPUT_AIR_TEMPERATURE = 1u << 1,
  EZO_HUM_OUTPUT_DEW_POINT = 1u << 2
};

typedef struct {
  ezo_hum_output_mask_t present_mask;
  double relative_humidity_percent;
  double air_temperature_c;
  double dew_point_c;
} ezo_hum_reading_t;

typedef struct {
  ezo_hum_output_mask_t enabled_mask;
} ezo_hum_output_config_t;

ezo_result_t ezo_hum_parse_reading(const char *buffer,
                                   size_t buffer_len,
                                   ezo_hum_output_mask_t enabled_mask,
                                   ezo_hum_reading_t *reading_out);

ezo_result_t ezo_hum_parse_output_config(const char *buffer,
                                         size_t buffer_len,
                                         ezo_hum_output_config_t *config_out);

ezo_result_t ezo_hum_build_output_command(char *buffer,
                                          size_t buffer_len,
                                          ezo_hum_output_mask_t output,
                                          uint8_t enabled);

ezo_result_t ezo_hum_send_read_i2c(ezo_i2c_device_t *device,
                                   ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_hum_send_output_query_i2c(ezo_i2c_device_t *device,
                                           ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_hum_send_output_set_i2c(ezo_i2c_device_t *device,
                                         ezo_hum_output_mask_t output,
                                         uint8_t enabled,
                                         ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_hum_read_response_i2c(ezo_i2c_device_t *device,
                                       ezo_hum_output_mask_t enabled_mask,
                                       ezo_hum_reading_t *reading_out);

ezo_result_t ezo_hum_read_output_config_i2c(ezo_i2c_device_t *device,
                                            ezo_hum_output_config_t *config_out);

ezo_result_t ezo_hum_send_read_uart(ezo_uart_device_t *device,
                                    ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_hum_send_output_query_uart(ezo_uart_device_t *device,
                                            ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_hum_send_output_set_uart(ezo_uart_device_t *device,
                                          ezo_hum_output_mask_t output,
                                          uint8_t enabled,
                                          ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_hum_read_response_uart(ezo_uart_device_t *device,
                                        ezo_hum_output_mask_t enabled_mask,
                                        ezo_hum_reading_t *reading_out);

ezo_result_t ezo_hum_read_output_config_uart(ezo_uart_device_t *device,
                                             ezo_hum_output_config_t *config_out);

#ifdef __cplusplus
}
#endif

#endif
