#ifndef EZO_UART_ARDUINO_STREAM_H
#define EZO_UART_ARDUINO_STREAM_H

#ifndef __cplusplus
#error "uart_stream adapter requires C++"
#endif

#include <Arduino.h>

#include "ezo_uart.h"

struct ezo_uart_arduino_stream_context_t {
  Stream *stream;
};

ezo_result_t ezo_uart_arduino_stream_context_init(
    ezo_uart_arduino_stream_context_t *context,
    Stream *stream);

const ezo_uart_transport_t *ezo_uart_arduino_stream_transport(void);

#endif
