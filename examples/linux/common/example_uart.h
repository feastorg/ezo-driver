#ifndef EZO_EXAMPLE_UART_H
#define EZO_EXAMPLE_UART_H

#include "ezo_control.h"
#include "ezo_uart_posix_serial.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  ezo_uart_posix_serial_t serial;
  ezo_uart_device_t device;
} ezo_example_uart_session_t;

ezo_result_t ezo_example_open_uart(const char *device_path,
                                   ezo_uart_posix_baud_t baud,
                                   ezo_example_uart_session_t *session_out);

void ezo_example_close_uart(ezo_example_uart_session_t *session);

ezo_result_t ezo_example_uart_bootstrap_response_codes(
    ezo_uart_device_t *device,
    ezo_product_id_t product_id,
    uint8_t *enabled_before_out,
    uint8_t *enabled_after_out);

#ifdef __cplusplus
}
#endif

#endif
