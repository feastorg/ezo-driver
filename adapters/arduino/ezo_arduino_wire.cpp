#include "ezo_i2c/adapters/arduino_wire.h"

#include <stddef.h>
#include <stdint.h>

static ezo_result_t ezo_arduino_write_then_read(void *context,
                                                uint8_t address,
                                                const uint8_t *tx_data,
                                                size_t tx_len,
                                                uint8_t *rx_data,
                                                size_t rx_len,
                                                size_t *rx_received) {
  ezo_arduino_wire_context_t *arduino_context =
      static_cast<ezo_arduino_wire_context_t *>(context);
  TwoWire *wire = NULL;
  size_t requested = 0;
  size_t received = 0;

  if (arduino_context == NULL || arduino_context->wire == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if ((tx_len > 0 && tx_data == NULL) || (rx_len > 0 && rx_data == NULL)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  wire = arduino_context->wire;

  if (tx_len > 0) {
    wire->beginTransmission(address);
    if (wire->write(tx_data, tx_len) != tx_len) {
      wire->endTransmission();
      if (rx_received != NULL) {
        *rx_received = 0;
      }
      return EZO_ERR_TRANSPORT;
    }

    if (wire->endTransmission() != 0) {
      if (rx_received != NULL) {
        *rx_received = 0;
      }
      return EZO_ERR_TRANSPORT;
    }
  }

  if (rx_received != NULL) {
    *rx_received = 0;
  }

  if (rx_len == 0) {
    return EZO_OK;
  }

  requested = static_cast<size_t>(wire->requestFrom(static_cast<int>(address),
                                                    static_cast<int>(rx_len),
                                                    static_cast<int>(1)));

  if (requested > rx_len) {
    requested = rx_len;
  }

  for (size_t i = 0; i < requested && wire->available(); ++i) {
    rx_data[i] = static_cast<uint8_t>(wire->read());
    received += 1;
  }

  if (rx_received != NULL) {
    *rx_received = received;
  }

  return EZO_OK;
}

ezo_result_t ezo_arduino_wire_context_init(ezo_arduino_wire_context_t *context, TwoWire *wire) {
  if (context == NULL || wire == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  context->wire = wire;
  return EZO_OK;
}

const ezo_i2c_transport_t *ezo_arduino_wire_transport(void) {
  static const ezo_i2c_transport_t transport = {
      ezo_arduino_write_then_read,
  };

  return &transport;
}
