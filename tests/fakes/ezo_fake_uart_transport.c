#include "tests/fakes/ezo_fake_uart_transport.h"

#include <string.h>

static ezo_result_t ezo_fake_uart_write_bytes(void *context,
                                              const uint8_t *tx_data,
                                              size_t tx_len) {
  ezo_fake_uart_transport_t *fake = (ezo_fake_uart_transport_t *)context;

  if (fake == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  fake->write_call_count += 1;

  if (fake->write_result != EZO_OK) {
    return fake->write_result;
  }

  if ((tx_len > 0 && tx_data == NULL) || fake->tx_len + tx_len > EZO_FAKE_UART_MAX_TX) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (tx_len > 0) {
    memcpy(&fake->tx_bytes[fake->tx_len], tx_data, tx_len);
    fake->tx_len += tx_len;
  }

  return EZO_OK;
}

static ezo_result_t ezo_fake_uart_read_bytes(void *context,
                                             uint8_t *rx_data,
                                             size_t rx_len,
                                             size_t *rx_received) {
  ezo_fake_uart_transport_t *fake = (ezo_fake_uart_transport_t *)context;
  size_t remaining = 0;
  size_t to_copy = 0;

  if (fake == NULL || rx_received == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  fake->read_call_count += 1;
  *rx_received = 0;

  if (fake->read_result != EZO_OK) {
    return fake->read_result;
  }

  if ((rx_len > 0 && rx_data == NULL) || rx_len == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (fake->response_offset >= fake->response_len) {
    return EZO_OK;
  }

  remaining = fake->response_len - fake->response_offset;
  to_copy = remaining;
  if (to_copy > rx_len) {
    to_copy = rx_len;
  }
  if (fake->max_bytes_per_read > 0 && to_copy > fake->max_bytes_per_read) {
    to_copy = fake->max_bytes_per_read;
  }

  memcpy(rx_data, &fake->response_bytes[fake->response_offset], to_copy);
  fake->response_offset += to_copy;
  *rx_received = to_copy;
  return EZO_OK;
}

static ezo_result_t ezo_fake_uart_discard_input(void *context) {
  ezo_fake_uart_transport_t *fake = (ezo_fake_uart_transport_t *)context;

  if (fake == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  fake->discard_call_count += 1;

  if (fake->discard_result != EZO_OK) {
    return fake->discard_result;
  }

  fake->response_offset = fake->response_len;
  return EZO_OK;
}

static void ezo_fake_uart_transport_compact_response(ezo_fake_uart_transport_t *fake) {
  size_t unread_len = 0;

  if (fake == NULL || fake->response_offset == 0) {
    return;
  }

  if (fake->response_offset >= fake->response_len) {
    memset(fake->response_bytes, 0, sizeof(fake->response_bytes));
    fake->response_len = 0;
    fake->response_offset = 0;
    return;
  }

  unread_len = fake->response_len - fake->response_offset;
  memmove(fake->response_bytes, &fake->response_bytes[fake->response_offset], unread_len);
  memset(&fake->response_bytes[unread_len], 0, sizeof(fake->response_bytes) - unread_len);
  fake->response_len = unread_len;
  fake->response_offset = 0;
}

void ezo_fake_uart_transport_init(ezo_fake_uart_transport_t *fake) {
  if (fake == NULL) {
    return;
  }

  memset(fake, 0, sizeof(*fake));
  fake->write_result = EZO_OK;
  fake->read_result = EZO_OK;
  fake->discard_result = EZO_OK;
}

void ezo_fake_uart_transport_set_response(ezo_fake_uart_transport_t *fake,
                                          const uint8_t *response_bytes,
                                          size_t response_len) {
  if (fake == NULL) {
    return;
  }

  memset(fake->response_bytes, 0, sizeof(fake->response_bytes));
  fake->response_len = 0;
  fake->response_offset = 0;

  if (response_bytes == NULL || response_len == 0) {
    return;
  }

  if (response_len > EZO_FAKE_UART_MAX_RX) {
    response_len = EZO_FAKE_UART_MAX_RX;
  }

  memcpy(fake->response_bytes, response_bytes, response_len);
  fake->response_len = response_len;
}

void ezo_fake_uart_transport_append_response(ezo_fake_uart_transport_t *fake,
                                             const uint8_t *response_bytes,
                                             size_t response_len) {
  size_t available = 0;

  if (fake == NULL || response_bytes == NULL || response_len == 0) {
    return;
  }

  ezo_fake_uart_transport_compact_response(fake);

  if (fake->response_len >= EZO_FAKE_UART_MAX_RX) {
    return;
  }

  available = EZO_FAKE_UART_MAX_RX - fake->response_len;
  if (response_len > available) {
    response_len = available;
  }

  memcpy(&fake->response_bytes[fake->response_len], response_bytes, response_len);
  fake->response_len += response_len;
}

const ezo_uart_transport_t *ezo_fake_uart_transport_vtable(void) {
  static const ezo_uart_transport_t transport = {
      ezo_fake_uart_write_bytes,
      ezo_fake_uart_read_bytes,
      ezo_fake_uart_discard_input,
  };

  return &transport;
}
