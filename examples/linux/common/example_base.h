#ifndef EZO_EXAMPLE_BASE_H
#define EZO_EXAMPLE_BASE_H

#include "ezo.h"
#include "ezo_product.h"
#include "ezo_uart_posix_serial.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EZO_EXAMPLE_I2C_DEFAULT_PATH "/dev/i2c-1"
#define EZO_EXAMPLE_UART_DEFAULT_PATH "/dev/ttyUSB0"
#define EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE 9600U

typedef struct {
  const char *device_path;
  uint8_t address;
} ezo_example_i2c_options_t;

typedef struct {
  const char *device_path;
  ezo_uart_posix_baud_t baud;
  uint32_t baud_rate;
} ezo_example_uart_options_t;

int ezo_example_parse_uint32_arg(const char *text, uint32_t *value_out);

int ezo_example_parse_uint8_arg(const char *text, uint8_t *value_out);

int ezo_example_parse_double_arg(const char *text, double *value_out);

int ezo_example_parse_baud_arg(const char *text,
                               ezo_uart_posix_baud_t *baud_out,
                               uint32_t *baud_rate_out);

int ezo_example_parse_product_id_arg(const char *text, ezo_product_id_t *product_id_out);

int ezo_example_parse_i2c_options(int argc,
                                  char **argv,
                                  uint8_t default_address,
                                  ezo_example_i2c_options_t *options_out,
                                  int *next_arg_out);

int ezo_example_parse_uart_options(int argc,
                                   char **argv,
                                   uint32_t default_baud_rate,
                                   ezo_example_uart_options_t *options_out,
                                   int *next_arg_out);

int ezo_example_has_flag(int argc, char **argv, int start_index, const char *flag);

const char *ezo_example_find_option_value(int argc,
                                          char **argv,
                                          int start_index,
                                          const char *prefix);

void ezo_example_wait_hint(const ezo_timing_hint_t *hint);

void ezo_example_sleep_ms(uint32_t delay_ms);

int ezo_example_print_error(const char *step, ezo_result_t result);

#ifdef __cplusplus
}
#endif

#endif
