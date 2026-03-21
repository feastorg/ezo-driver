#ifndef EZO_DO_H
#define EZO_DO_H

#include "ezo_i2c.h"
#include "ezo_uart.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ezo_do_output_mask_t;

enum {
  EZO_DO_OUTPUT_MG_L = 1u << 0,
  EZO_DO_OUTPUT_PERCENT_SATURATION = 1u << 1
};

typedef enum {
  EZO_DO_SALINITY_UNIT_MICROSIEMENS = 0,
  EZO_DO_SALINITY_UNIT_PPT
} ezo_do_salinity_unit_t;

typedef struct {
  ezo_do_output_mask_t present_mask;
  double milligrams_per_liter;
  double percent_saturation;
} ezo_do_reading_t;

typedef struct {
  ezo_do_output_mask_t enabled_mask;
} ezo_do_output_config_t;

typedef struct {
  double temperature_c;
} ezo_do_temperature_compensation_t;

typedef struct {
  double value;
  ezo_do_salinity_unit_t unit;
} ezo_do_salinity_compensation_t;

typedef struct {
  double pressure_kpa;
} ezo_do_pressure_compensation_t;

ezo_result_t ezo_do_parse_reading(const char *buffer,
                                  size_t buffer_len,
                                  ezo_do_output_mask_t enabled_mask,
                                  ezo_do_reading_t *reading_out);

ezo_result_t ezo_do_parse_output_config(const char *buffer,
                                        size_t buffer_len,
                                        ezo_do_output_config_t *config_out);

ezo_result_t ezo_do_parse_temperature(const char *buffer,
                                      size_t buffer_len,
                                      ezo_do_temperature_compensation_t *temperature_out);

ezo_result_t ezo_do_parse_salinity(const char *buffer,
                                   size_t buffer_len,
                                   ezo_do_salinity_compensation_t *salinity_out);

ezo_result_t ezo_do_parse_pressure(const char *buffer,
                                   size_t buffer_len,
                                   ezo_do_pressure_compensation_t *pressure_out);

ezo_result_t ezo_do_build_output_command(char *buffer,
                                         size_t buffer_len,
                                         ezo_do_output_mask_t output,
                                         uint8_t enabled);

ezo_result_t ezo_do_build_temperature_command(char *buffer,
                                              size_t buffer_len,
                                              double temperature_c,
                                              uint8_t decimals);

ezo_result_t ezo_do_build_salinity_command(char *buffer,
                                           size_t buffer_len,
                                           double value,
                                           ezo_do_salinity_unit_t unit,
                                           uint8_t decimals);

ezo_result_t ezo_do_build_pressure_command(char *buffer,
                                           size_t buffer_len,
                                           double pressure_kpa,
                                           uint8_t decimals);

ezo_result_t ezo_do_send_read_i2c(ezo_i2c_device_t *device,
                                  ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device,
                                                 double temperature_c,
                                                 uint8_t decimals,
                                                 ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_output_query_i2c(ezo_i2c_device_t *device,
                                          ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_output_set_i2c(ezo_i2c_device_t *device,
                                        ezo_do_output_mask_t output,
                                        uint8_t enabled,
                                        ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_temperature_query_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_temperature_set_i2c(ezo_i2c_device_t *device,
                                             double temperature_c,
                                             uint8_t decimals,
                                             ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_salinity_query_i2c(ezo_i2c_device_t *device,
                                            ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_salinity_set_i2c(ezo_i2c_device_t *device,
                                          double value,
                                          ezo_do_salinity_unit_t unit,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_pressure_query_i2c(ezo_i2c_device_t *device,
                                            ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_pressure_set_i2c(ezo_i2c_device_t *device,
                                          double pressure_kpa,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_read_response_i2c(ezo_i2c_device_t *device,
                                      ezo_do_output_mask_t enabled_mask,
                                      ezo_do_reading_t *reading_out);

ezo_result_t ezo_do_read_output_config_i2c(ezo_i2c_device_t *device,
                                           ezo_do_output_config_t *config_out);

ezo_result_t ezo_do_read_temperature_i2c(ezo_i2c_device_t *device,
                                         ezo_do_temperature_compensation_t *temperature_out);

ezo_result_t ezo_do_read_salinity_i2c(ezo_i2c_device_t *device,
                                      ezo_do_salinity_compensation_t *salinity_out);

ezo_result_t ezo_do_read_pressure_i2c(ezo_i2c_device_t *device,
                                      ezo_do_pressure_compensation_t *pressure_out);

ezo_result_t ezo_do_send_read_uart(ezo_uart_device_t *device,
                                   ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_read_with_temp_comp_uart(ezo_uart_device_t *device,
                                                  double temperature_c,
                                                  uint8_t decimals,
                                                  ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_output_query_uart(ezo_uart_device_t *device,
                                           ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_output_set_uart(ezo_uart_device_t *device,
                                         ezo_do_output_mask_t output,
                                         uint8_t enabled,
                                         ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_temperature_query_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_temperature_set_uart(ezo_uart_device_t *device,
                                              double temperature_c,
                                              uint8_t decimals,
                                              ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_salinity_query_uart(ezo_uart_device_t *device,
                                             ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_salinity_set_uart(ezo_uart_device_t *device,
                                           double value,
                                           ezo_do_salinity_unit_t unit,
                                           uint8_t decimals,
                                           ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_pressure_query_uart(ezo_uart_device_t *device,
                                             ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_send_pressure_set_uart(ezo_uart_device_t *device,
                                           double pressure_kpa,
                                           uint8_t decimals,
                                           ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_do_read_response_uart(ezo_uart_device_t *device,
                                       ezo_do_output_mask_t enabled_mask,
                                       ezo_do_reading_t *reading_out);

ezo_result_t ezo_do_read_output_config_uart(ezo_uart_device_t *device,
                                            ezo_do_output_config_t *config_out);

ezo_result_t ezo_do_read_temperature_uart(
    ezo_uart_device_t *device,
    ezo_do_temperature_compensation_t *temperature_out);

ezo_result_t ezo_do_read_salinity_uart(ezo_uart_device_t *device,
                                       ezo_do_salinity_compensation_t *salinity_out);

ezo_result_t ezo_do_read_pressure_uart(ezo_uart_device_t *device,
                                       ezo_do_pressure_compensation_t *pressure_out);

#ifdef __cplusplus
}
#endif

#endif
