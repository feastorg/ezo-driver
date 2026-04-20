#include "ezo_uart_posix_serial.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

static ezo_result_t ezo_uart_posix_map_baud(ezo_uart_posix_baud_t baud, speed_t *speed_out) {
  if (speed_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  switch (baud) {
  case EZO_UART_POSIX_BAUD_300:
    *speed_out = B300;
    return EZO_OK;
  case EZO_UART_POSIX_BAUD_1200:
    *speed_out = B1200;
    return EZO_OK;
  case EZO_UART_POSIX_BAUD_2400:
    *speed_out = B2400;
    return EZO_OK;
  case EZO_UART_POSIX_BAUD_9600:
    *speed_out = B9600;
    return EZO_OK;
  case EZO_UART_POSIX_BAUD_19200:
    *speed_out = B19200;
    return EZO_OK;
  case EZO_UART_POSIX_BAUD_38400:
    *speed_out = B38400;
    return EZO_OK;
  case EZO_UART_POSIX_BAUD_57600:
    *speed_out = B57600;
    return EZO_OK;
  case EZO_UART_POSIX_BAUD_115200:
    *speed_out = B115200;
    return EZO_OK;
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }
}


static int ezo_uart_posix_serial_is_open(const ezo_uart_posix_serial_t *serial) {
  return serial != NULL && serial->fd >= 0 && serial->owns_fd != 0 &&
         serial->has_saved_termios != 0 && serial->read_timeout_ms > 0;
}

static void ezo_uart_posix_serial_reset(ezo_uart_posix_serial_t *serial) {
  if (serial == NULL) {
    return;
  }

  serial->fd = -1;
  serial->read_timeout_ms = 0;
  serial->owns_fd = 0;
  serial->has_saved_termios = 0;
}

static ezo_result_t ezo_uart_posix_serial_open_fresh(ezo_uart_posix_serial_t *serial,
                                                     const char *path,
                                                     ezo_uart_posix_baud_t baud,
                                                     uint32_t read_timeout_ms) {
  struct termios configured_termios;
  speed_t speed = 0;
  int fd = -1;
  ezo_result_t result = EZO_OK;

  if (serial == NULL || path == NULL || path[0] == '\0') {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (read_timeout_ms == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  ezo_uart_posix_serial_reset(serial);

  result = ezo_uart_posix_map_baud(baud, &speed);
  if (result != EZO_OK) {
    return result;
  }

  fd = open(path, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    return EZO_ERR_TRANSPORT;
  }

  if (tcgetattr(fd, &serial->saved_termios) != 0) {
    close(fd);
    return EZO_ERR_TRANSPORT;
  }

  configured_termios = serial->saved_termios;
  configured_termios.c_iflag &=
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
  configured_termios.c_oflag &= ~OPOST;
  configured_termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  configured_termios.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
  configured_termios.c_cflag |= (CS8 | CLOCAL | CREAD);
#ifdef CRTSCTS
  configured_termios.c_cflag &= ~CRTSCTS;
#endif
  /* VMIN=0, VTIME=0: non-blocking read; timeout is handled via poll() in
   * ezo_uart_posix_read_bytes, which works correctly on both real serial
   * hardware and PTY devices (VTIME is unreliable on PTY line discipline). */
  configured_termios.c_cc[VMIN] = 0;
  configured_termios.c_cc[VTIME] = 0;

  if (cfsetispeed(&configured_termios, speed) != 0 ||
      cfsetospeed(&configured_termios, speed) != 0 ||
      tcsetattr(fd, TCSANOW, &configured_termios) != 0) {
    close(fd);
    return EZO_ERR_TRANSPORT;
  }

  serial->fd = fd;
  serial->read_timeout_ms = read_timeout_ms;
  serial->owns_fd = 1;
  serial->has_saved_termios = 1;
  return EZO_OK;
}

static ezo_result_t ezo_uart_posix_write_bytes(void *context,
                                               const uint8_t *tx_data,
                                               size_t tx_len) {
  ezo_uart_posix_serial_t *serial = (ezo_uart_posix_serial_t *)context;
  size_t written = 0;

  if (serial == NULL || serial->fd < 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (tx_len > 0 && tx_data == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  while (written < tx_len) {
    ssize_t io_result = write(serial->fd, tx_data + written, tx_len - written);
    if (io_result < 0) {
      if (errno == EINTR) {
        continue;
      }
      return EZO_ERR_TRANSPORT;
    }

    if (io_result == 0) {
      return EZO_ERR_TRANSPORT;
    }

    written += (size_t)io_result;
  }

  if (tcdrain(serial->fd) != 0) {
    return EZO_ERR_TRANSPORT;
  }

  return EZO_OK;
}

static ezo_result_t ezo_uart_posix_read_bytes(void *context,
                                              uint8_t *rx_data,
                                              size_t rx_len,
                                              size_t *rx_received) {
  ezo_uart_posix_serial_t *serial = (ezo_uart_posix_serial_t *)context;
  struct pollfd pfd;
  int poll_result = 0;

  if (serial == NULL || serial->fd < 0 || rx_received == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (rx_len > 0 && rx_data == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  *rx_received = 0;

  if (rx_len == 0) {
    return EZO_OK;
  }

  /* Use poll() for the read timeout. VTIME is unreliable on PTY devices
   * (it is implemented in the hardware UART driver, not the PTY line
   * discipline). poll() works correctly on both real hardware and PTY. */
  pfd.fd = serial->fd;
  pfd.events = POLLIN;
  pfd.revents = 0;

  do {
    poll_result = poll(&pfd, 1, (int)serial->read_timeout_ms);
  } while (poll_result < 0 && errno == EINTR);

  if (poll_result < 0) {
    return EZO_ERR_TRANSPORT;
  }

  if (poll_result == 0) {
    /* Timeout expired — no data available. */
    return EZO_OK;
  }

  if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    return EZO_ERR_TRANSPORT;
  }

  for (;;) {
    ssize_t io_result = read(serial->fd, rx_data, rx_len);
    if (io_result < 0) {
      if (errno == EINTR) {
        continue;
      }
      return EZO_ERR_TRANSPORT;
    }

    *rx_received = (size_t)io_result;
    return EZO_OK;
  }
}

static ezo_result_t ezo_uart_posix_discard_input(void *context) {
  ezo_uart_posix_serial_t *serial = (ezo_uart_posix_serial_t *)context;

  if (serial == NULL || serial->fd < 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (tcflush(serial->fd, TCIFLUSH) != 0) {
    return EZO_ERR_TRANSPORT;
  }

  return EZO_OK;
}

ezo_result_t ezo_uart_posix_serial_open(ezo_uart_posix_serial_t *serial,
                                        const char *path,
                                        ezo_uart_posix_baud_t baud,
                                        uint32_t read_timeout_ms) {
  return ezo_uart_posix_serial_open_fresh(serial, path, baud, read_timeout_ms);
}

void ezo_uart_posix_serial_close(ezo_uart_posix_serial_t *serial) {
  if (serial == NULL) {
    return;
  }

  if (ezo_uart_posix_serial_is_open(serial)) {
    tcsetattr(serial->fd, TCSANOW, &serial->saved_termios);
    close(serial->fd);
  }

  ezo_uart_posix_serial_reset(serial);
}

const ezo_uart_transport_t *ezo_uart_posix_serial_transport(void) {
  static const ezo_uart_transport_t transport = {
      ezo_uart_posix_write_bytes,
      ezo_uart_posix_read_bytes,
      ezo_uart_posix_discard_input,
  };

  return &transport;
}
