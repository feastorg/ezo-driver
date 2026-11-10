#ifndef EZO_O2_H
#define EZO_O2_H

#include "ezo_i2c.h"
#include "ezo_uart.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ezo_o2_output_mask_t;

enum {
  EZO_O2_OUTPUT_PERCENT = 1u << 0,
  EZO_O2_OUTPUT_PPT = 1u << 1
};

typedef struct {
  ezo_o2_output_mask_t present_mask;
  double percent;
  double parts_per_thousand;
} ezo_o2_reading_t;

typedef struct {
  ezo_o2_output_mask_t enabled_mask;
} ezo_o2_output_config_t;

typedef struct {
  double temperature_c;
} ezo_o2_temperature_compensation_t;

/* A reading with both outputs enabled is parsed as percent, then parts per thousand. That order
 * is inferred from the `?,O,%,PPT` query reply; no datasheet page prints a two-value reading. */
ezo_result_t ezo_o2_parse_reading(const char *buffer,
                                  size_t buffer_len,
                                  ezo_o2_output_mask_t enabled_mask,
                                  ezo_o2_reading_t *reading_out);

ezo_result_t ezo_o2_parse_output_config(const char *buffer,
                                        size_t buffer_len,
                                        ezo_o2_output_config_t *config_out);

ezo_result_t ezo_o2_parse_temperature(const char *buffer,
                                      size_t buffer_len,
                                      ezo_o2_temperature_compensation_t *temperature_out);

ezo_result_t ezo_o2_build_output_command(char *buffer,
                                         size_t buffer_len,
                                         ezo_o2_output_mask_t output,
                                         uint8_t enabled);

ezo_result_t ezo_o2_build_temperature_command(char *buffer,
                                              size_t buffer_len,
                                              double temperature_c,
                                              uint8_t decimals);

ezo_result_t ezo_o2_send_read_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_read_with_temp_comp_i2c(ezo_i2c_device_t *device,
                                                 double temperature_c,
                                                 uint8_t decimals,
                                                 ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_output_query_i2c(ezo_i2c_device_t *device,
                                          ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_output_set_i2c(ezo_i2c_device_t *device,
                                        ezo_o2_output_mask_t output,
                                        uint8_t enabled,
                                        ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_temperature_query_i2c(ezo_i2c_device_t *device,
                                               ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_temperature_set_i2c(ezo_i2c_device_t *device,
                                             double temperature_c,
                                             uint8_t decimals,
                                             ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_read_response_i2c(ezo_i2c_device_t *device,
                                      ezo_o2_output_mask_t enabled_mask,
                                      ezo_o2_reading_t *reading_out);

ezo_result_t ezo_o2_read_output_config_i2c(ezo_i2c_device_t *device,
                                           ezo_o2_output_config_t *config_out);

ezo_result_t ezo_o2_read_temperature_i2c(ezo_i2c_device_t *device,
                                         ezo_o2_temperature_compensation_t *temperature_out);

ezo_result_t ezo_o2_send_read_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_read_with_temp_comp_uart(ezo_uart_device_t *device,
                                                  double temperature_c,
                                                  uint8_t decimals,
                                                  ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_output_query_uart(ezo_uart_device_t *device,
                                           ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_output_set_uart(ezo_uart_device_t *device,
                                         ezo_o2_output_mask_t output,
                                         uint8_t enabled,
                                         ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_temperature_query_uart(ezo_uart_device_t *device,
                                                ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_send_temperature_set_uart(ezo_uart_device_t *device,
                                              double temperature_c,
                                              uint8_t decimals,
                                              ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_o2_read_response_uart(ezo_uart_device_t *device,
                                       ezo_o2_output_mask_t enabled_mask,
                                       ezo_o2_reading_t *reading_out);

/* Over UART, RT replies `*OK` and then the value, the reverse of R. */
ezo_result_t ezo_o2_read_response_with_temp_comp_uart(ezo_uart_device_t *device,
                                                      ezo_o2_output_mask_t enabled_mask,
                                                      ezo_o2_reading_t *reading_out);

ezo_result_t ezo_o2_read_output_config_uart(ezo_uart_device_t *device,
                                            ezo_o2_output_config_t *config_out);

ezo_result_t ezo_o2_read_temperature_uart(ezo_uart_device_t *device,
                                          ezo_o2_temperature_compensation_t *temperature_out);

#ifdef __cplusplus
}
#endif

#endif
