#ifndef EZO_CO2_H
#define EZO_CO2_H

#include "ezo_i2c.h"
#include "ezo_uart.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ezo_co2_output_mask_t;

enum {
  EZO_CO2_OUTPUT_PPM = 1u << 0,
  EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE = 1u << 1
};

typedef struct {
  ezo_co2_output_mask_t present_mask;
  double ppm;
  double internal_temperature_c;
  uint8_t warming;
} ezo_co2_reading_t;

typedef struct {
  ezo_co2_output_mask_t enabled_mask;
} ezo_co2_output_config_t;

/* A reading with both outputs enabled is parsed as ppm, then internal temperature. That order is
 * inferred from the `?O,ppm,t` query reply; no datasheet page prints a two-value reading. For
 * the 10 s after power-up or wake the circuit outputs `*warm`: the result is EZO_OK with
 * warming set and no outputs present. */
ezo_result_t ezo_co2_parse_reading(const char *buffer,
                                   size_t buffer_len,
                                   ezo_co2_output_mask_t enabled_mask,
                                   ezo_co2_reading_t *reading_out);

ezo_result_t ezo_co2_parse_output_config(const char *buffer,
                                         size_t buffer_len,
                                         ezo_co2_output_config_t *config_out);

ezo_result_t ezo_co2_build_output_command(char *buffer,
                                          size_t buffer_len,
                                          ezo_co2_output_mask_t output,
                                          uint8_t enabled);

ezo_result_t ezo_co2_send_read_i2c(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_co2_send_output_query_i2c(ezo_i2c_device_t *device,
                                           ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_co2_send_output_set_i2c(ezo_i2c_device_t *device,
                                         ezo_co2_output_mask_t output,
                                         uint8_t enabled,
                                         ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_co2_read_response_i2c(ezo_i2c_device_t *device,
                                       ezo_co2_output_mask_t enabled_mask,
                                       ezo_co2_reading_t *reading_out);

ezo_result_t ezo_co2_read_output_config_i2c(ezo_i2c_device_t *device,
                                            ezo_co2_output_config_t *config_out);

ezo_result_t ezo_co2_send_read_uart(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_co2_send_output_query_uart(ezo_uart_device_t *device,
                                            ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_co2_send_output_set_uart(ezo_uart_device_t *device,
                                          ezo_co2_output_mask_t output,
                                          uint8_t enabled,
                                          ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_co2_read_response_uart(ezo_uart_device_t *device,
                                        ezo_co2_output_mask_t enabled_mask,
                                        ezo_co2_reading_t *reading_out);

ezo_result_t ezo_co2_read_output_config_uart(ezo_uart_device_t *device,
                                             ezo_co2_output_config_t *config_out);

#ifdef __cplusplus
}
#endif

#endif
