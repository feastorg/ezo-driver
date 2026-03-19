#include "ezo_i2c/adapters/linux_i2c.h"

#include <stddef.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/i2c-dev.h>

static ezo_result_t ezo_linux_write_then_read(void *context,
                                              uint8_t address,
                                              const uint8_t *tx_data,
                                              size_t tx_len,
                                              uint8_t *rx_data,
                                              size_t rx_len,
                                              size_t *rx_received) {
  ezo_linux_i2c_context_t *linux_context = (ezo_linux_i2c_context_t *)context;
  ssize_t io_result = 0;

  if (linux_context == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if ((tx_len > 0 && tx_data == NULL) || (rx_len > 0 && rx_data == NULL)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (ioctl(linux_context->fd, I2C_SLAVE, address) < 0) {
    if (rx_received != NULL) {
      *rx_received = 0;
    }
    return EZO_ERR_TRANSPORT;
  }

  if (tx_len > 0) {
    io_result = write(linux_context->fd, tx_data, tx_len);
    if (io_result < 0 || (size_t)io_result != tx_len) {
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

  io_result = read(linux_context->fd, rx_data, rx_len);
  if (io_result < 0) {
    return EZO_ERR_TRANSPORT;
  }

  if (rx_received != NULL) {
    *rx_received = (size_t)io_result;
  }

  return EZO_OK;
}

ezo_result_t ezo_linux_i2c_context_init(ezo_linux_i2c_context_t *context, int fd) {
  if (context == NULL || fd < 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  context->fd = fd;
  return EZO_OK;
}

const ezo_i2c_transport_t *ezo_linux_i2c_transport(void) {
  static const ezo_i2c_transport_t transport = {
      ezo_linux_write_then_read,
  };

  return &transport;
}
