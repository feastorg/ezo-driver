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

int ezo_example_print_error(const char *step, ezo_result_t result);

const char *ezo_example_bool_name(int value);
const char *ezo_example_product_name(ezo_product_id_t product_id);
const char *ezo_example_transport_name(ezo_product_transport_t transport);
const char *ezo_example_support_name(ezo_product_support_t support);
const char *ezo_example_default_state_name(ezo_product_default_state_t state);
const char *ezo_example_output_schema_name(ezo_product_output_schema_t schema);
const char *ezo_example_uart_response_kind_name(
    ezo_uart_response_kind_t response_kind);

void ezo_example_print_device_info(const ezo_device_info_t *info);
void ezo_example_print_product_metadata(const ezo_product_metadata_t *metadata);

#ifdef __cplusplus
}
#endif

#endif
