#ifndef EZO_EC_H
#define EZO_EC_H

#include "ezo_i2c.h"
#include "ezo_uart.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ezo_ec_output_mask_t;

enum {
  EZO_EC_OUTPUT_CONDUCTIVITY = 1u << 0,
  EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS = 1u << 1,
  EZO_EC_OUTPUT_SALINITY = 1u << 2,
  EZO_EC_OUTPUT_SPECIFIC_GRAVITY = 1u << 3
};

typedef struct {
  ezo_ec_output_mask_t present_mask;
  double conductivity_us_cm;
  double total_dissolved_solids_ppm;
  double salinity_ppt;
  double specific_gravity;
} ezo_ec_reading_t;

typedef struct {
  ezo_ec_output_mask_t enabled_mask;
} ezo_ec_output_config_t;

typedef struct {
  double temperature_c;
} ezo_ec_temperature_compensation_t;

typedef struct {
  double k_value;
} ezo_ec_probe_k_t;

typedef struct {
  double factor;
} ezo_ec_tds_factor_t;

ezo_result_t ezo_ec_parse_reading(const char *buffer,
                                  size_t buffer_len,
                                  ezo_ec_output_mask_t enabled_mask,
                                  ezo_ec_reading_t *reading_out);

ezo_result_t ezo_ec_parse_output_config(const char *buffer,
                                        size_t buffer_len,
                                        ezo_ec_output_config_t *config_out);

ezo_result_t ezo_ec_parse_temperature(const char *buffer,
                                      size_t buffer_len,
                                      ezo_ec_temperature_compensation_t *temperature_out);

ezo_result_t ezo_ec_parse_probe_k(const char *buffer,
                                  size_t buffer_len,
                                  ezo_ec_probe_k_t *probe_k_out);

ezo_result_t ezo_ec_parse_tds_factor(const char *buffer,
                                     size_t buffer_len,
                                     ezo_ec_tds_factor_t *tds_factor_out);

ezo_result_t ezo_ec_build_output_command(char *buffer,
                                         size_t buffer_len,
                                         ezo_ec_output_mask_t output,
                                         uint8_t enabled);

ezo_result_t ezo_ec_build_temperature_command(char *buffer,
                                              size_t buffer_len,
                                              double temperature_c,
                                              uint8_t decimals);

ezo_result_t ezo_ec_build_probe_k_command(char *buffer,
                                          size_t buffer_len,
                                          double k_value,
                                          uint8_t decimals);

ezo_result_t ezo_ec_build_tds_factor_command(char *buffer,
                                             size_t buffer_len,
                                             double factor,
                                             uint8_t decimals);

ezo_result_t ezo_ec_send_read_i2c(ezo_i2c_device_t *device,
                                  ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device,
                                                 double temperature_c,
                                                 uint8_t decimals,
                                                 ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_output_query_i2c(ezo_i2c_device_t *device,
                                          ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_output_set_i2c(ezo_i2c_device_t *device,
                                        ezo_ec_output_mask_t output,
                                        uint8_t enabled,
                                        ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_temperature_query_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_temperature_set_i2c(ezo_i2c_device_t *device,
                                             double temperature_c,
                                             uint8_t decimals,
                                             ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_probe_k_query_i2c(ezo_i2c_device_t *device,
                                           ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_probe_k_set_i2c(ezo_i2c_device_t *device,
                                         double k_value,
                                         uint8_t decimals,
                                         ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_tds_factor_query_i2c(ezo_i2c_device_t *device,
                                              ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_tds_factor_set_i2c(ezo_i2c_device_t *device,
                                            double factor,
                                            uint8_t decimals,
                                            ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_read_response_i2c(ezo_i2c_device_t *device,
                                      ezo_ec_output_mask_t enabled_mask,
                                      ezo_ec_reading_t *reading_out);

ezo_result_t ezo_ec_read_output_config_i2c(ezo_i2c_device_t *device,
                                           ezo_ec_output_config_t *config_out);

ezo_result_t ezo_ec_read_temperature_i2c(ezo_i2c_device_t *device,
                                         ezo_ec_temperature_compensation_t *temperature_out);

ezo_result_t ezo_ec_read_probe_k_i2c(ezo_i2c_device_t *device,
                                     ezo_ec_probe_k_t *probe_k_out);

ezo_result_t ezo_ec_read_tds_factor_i2c(ezo_i2c_device_t *device,
                                        ezo_ec_tds_factor_t *tds_factor_out);

ezo_result_t ezo_ec_send_read_uart(ezo_uart_device_t *device,
                                   ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_read_with_temp_comp_uart(ezo_uart_device_t *device,
                                                  double temperature_c,
                                                  uint8_t decimals,
                                                  ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_output_query_uart(ezo_uart_device_t *device,
                                           ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_output_set_uart(ezo_uart_device_t *device,
                                         ezo_ec_output_mask_t output,
                                         uint8_t enabled,
                                         ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_temperature_query_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_temperature_set_uart(ezo_uart_device_t *device,
                                              double temperature_c,
                                              uint8_t decimals,
                                              ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_probe_k_query_uart(ezo_uart_device_t *device,
                                            ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_probe_k_set_uart(ezo_uart_device_t *device,
                                          double k_value,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_tds_factor_query_uart(ezo_uart_device_t *device,
                                               ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_send_tds_factor_set_uart(ezo_uart_device_t *device,
                                             double factor,
                                             uint8_t decimals,
                                             ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_ec_read_response_uart(ezo_uart_device_t *device,
                                       ezo_ec_output_mask_t enabled_mask,
                                       ezo_ec_reading_t *reading_out);

ezo_result_t ezo_ec_read_output_config_uart(ezo_uart_device_t *device,
                                            ezo_ec_output_config_t *config_out);

ezo_result_t ezo_ec_read_temperature_uart(
    ezo_uart_device_t *device,
    ezo_ec_temperature_compensation_t *temperature_out);

ezo_result_t ezo_ec_read_probe_k_uart(ezo_uart_device_t *device,
                                      ezo_ec_probe_k_t *probe_k_out);

ezo_result_t ezo_ec_read_tds_factor_uart(ezo_uart_device_t *device,
                                         ezo_ec_tds_factor_t *tds_factor_out);

#ifdef __cplusplus
}
#endif

#endif
