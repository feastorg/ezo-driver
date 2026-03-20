#include "ezo_uart_arduino_stream.h"

#include <stddef.h>
#include <stdint.h>

static ezo_result_t ezo_uart_arduino_write_bytes(void *context,
                                                 const uint8_t *tx_data,
                                                 size_t tx_len) {
  ezo_uart_arduino_stream_context_t *arduino_context =
      static_cast<ezo_uart_arduino_stream_context_t *>(context);

  if (arduino_context == NULL || arduino_context->stream == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (tx_len > 0 && tx_data == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (tx_len == 0) {
    return EZO_OK;
  }

  if (arduino_context->stream->write(tx_data, tx_len) != tx_len) {
    return EZO_ERR_TRANSPORT;
  }

  return EZO_OK;
}

static ezo_result_t ezo_uart_arduino_read_bytes(void *context,
                                                uint8_t *rx_data,
                                                size_t rx_len,
                                                size_t *rx_received) {
  ezo_uart_arduino_stream_context_t *arduino_context =
      static_cast<ezo_uart_arduino_stream_context_t *>(context);
  size_t received = 0;

  if (arduino_context == NULL || arduino_context->stream == NULL || rx_received == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (rx_len > 0 && rx_data == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  *rx_received = 0;

  if (rx_len == 0) {
    return EZO_OK;
  }

  while (received < rx_len && arduino_context->stream->available() > 0) {
    int next = arduino_context->stream->read();
    if (next < 0) {
      break;
    }

    rx_data[received] = static_cast<uint8_t>(next);
    received += 1;
  }

  *rx_received = received;
  return EZO_OK;
}

static ezo_result_t ezo_uart_arduino_discard_input(void *context) {
  ezo_uart_arduino_stream_context_t *arduino_context =
      static_cast<ezo_uart_arduino_stream_context_t *>(context);

  if (arduino_context == NULL || arduino_context->stream == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  while (arduino_context->stream->available() > 0) {
    arduino_context->stream->read();
  }

  return EZO_OK;
}

ezo_result_t ezo_uart_arduino_stream_context_init(
    ezo_uart_arduino_stream_context_t *context,
    Stream *stream) {
  if (context == NULL || stream == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  context->stream = stream;
  return EZO_OK;
}

const ezo_uart_transport_t *ezo_uart_arduino_stream_transport(void) {
  static const ezo_uart_transport_t transport = {
      ezo_uart_arduino_write_bytes,
      ezo_uart_arduino_read_bytes,
      ezo_uart_arduino_discard_input,
  };

  return &transport;
}
