#ifndef EZO_UART_H
#define EZO_UART_H

#include "ezo.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EZO_UART_MAX_TEXT_RESPONSE_LEN 255
#define EZO_UART_MAX_TEXT_RESPONSE_CAPACITY (EZO_UART_MAX_TEXT_RESPONSE_LEN + 1U)

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

typedef struct ezo_uart_transport {
  ezo_result_t (*write_bytes)(void *context,
                              const uint8_t *tx_data,
                              size_t tx_len);
  ezo_result_t (*read_bytes)(void *context,
                             uint8_t *rx_data,
                             size_t rx_len,
                             size_t *rx_received);
  ezo_result_t (*discard_input)(void *context);
} ezo_uart_transport_t;

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

ezo_result_t ezo_uart_send_read(ezo_uart_device_t *device,
                                ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_uart_send_read_with_temp_comp(ezo_uart_device_t *device,
                                               double temperature_c,
                                               uint8_t decimals,
                                               ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_uart_read_line(ezo_uart_device_t *device,
                                char *buffer,
                                size_t buffer_len,
                                size_t *response_len,
                                ezo_uart_response_kind_t *response_kind);

int ezo_uart_response_kind_is_control(ezo_uart_response_kind_t response_kind);
int ezo_uart_response_kind_is_terminal(ezo_uart_response_kind_t response_kind);

ezo_result_t ezo_uart_discard_input(ezo_uart_device_t *device);

#ifdef __cplusplus
}
#endif

#endif
