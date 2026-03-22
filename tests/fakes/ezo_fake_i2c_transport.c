#include "tests/fakes/ezo_fake_i2c_transport.h"

#include <string.h>

static ezo_result_t ezo_fake_i2c_write_then_read(void *context,
                                                 uint8_t address,
                                                 const uint8_t *tx_data,
                                                 size_t tx_len,
                                                 uint8_t *rx_data,
                                                 size_t rx_len,
                                                 size_t *rx_received) {
  ezo_fake_i2c_transport_t *fake = (ezo_fake_i2c_transport_t *)context;
  size_t to_copy = 0;

  if (fake == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  fake->call_count += 1;
  fake->last_tx_len = 0;
  fake->last_rx_len_requested = rx_len;

  if (fake->enforce_expected_address && fake->expected_address != address) {
    if (rx_received != NULL) {
      *rx_received = 0;
    }
    return EZO_ERR_TRANSPORT;
  }

  if (tx_data != NULL && tx_len > 0) {
    if (tx_len > EZO_FAKE_I2C_TRANSPORT_MAX_TX) {
      return EZO_ERR_INVALID_ARGUMENT;
    }

    memcpy(fake->last_tx_bytes, tx_data, tx_len);
    fake->last_tx_len = tx_len;
  }

  if (fake->callback_result != EZO_OK) {
    if (rx_received != NULL) {
      *rx_received = 0;
    }
    return fake->callback_result;
  }

  if (rx_received != NULL) {
    *rx_received = 0;
  }

  if (rx_data == NULL || rx_len == 0 || fake->response_len == 0) {
    return EZO_OK;
  }

  to_copy = fake->response_len;
  if (to_copy > rx_len) {
    to_copy = rx_len;
  }

  memcpy(rx_data, fake->response_bytes, to_copy);

  if (rx_received != NULL) {
    *rx_received = to_copy;
  }

  return EZO_OK;
}

void ezo_fake_i2c_transport_init(ezo_fake_i2c_transport_t *fake) {
  if (fake == NULL) {
    return;
  }

  memset(fake, 0, sizeof(*fake));
  fake->callback_result = EZO_OK;
}

void ezo_fake_i2c_transport_set_response(ezo_fake_i2c_transport_t *fake,
                                         const uint8_t *response_bytes,
                                         size_t response_len) {
  if (fake == NULL) {
    return;
  }

  memset(fake->response_bytes, 0, sizeof(fake->response_bytes));
  fake->response_len = 0;

  if (response_bytes == NULL || response_len == 0) {
    return;
  }

  if (response_len > EZO_FAKE_I2C_TRANSPORT_MAX_RX) {
    response_len = EZO_FAKE_I2C_TRANSPORT_MAX_RX;
  }

  memcpy(fake->response_bytes, response_bytes, response_len);
  fake->response_len = response_len;
}

const ezo_i2c_transport_t *ezo_fake_i2c_transport_vtable(void) {
  static const ezo_i2c_transport_t transport = {
      ezo_fake_i2c_write_then_read,
  };

  return &transport;
}
