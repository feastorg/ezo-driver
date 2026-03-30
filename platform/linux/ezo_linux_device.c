#include "ezo_linux_device.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static ezo_result_t ezo_linux_uart_map_baud_rate(uint32_t baud_rate,
                                                 ezo_uart_posix_baud_t *baud_out) {
  if (baud_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  switch (baud_rate) {
  case 300:
    *baud_out = EZO_UART_POSIX_BAUD_300;
    return EZO_OK;
  case 1200:
    *baud_out = EZO_UART_POSIX_BAUD_1200;
    return EZO_OK;
  case 2400:
    *baud_out = EZO_UART_POSIX_BAUD_2400;
    return EZO_OK;
  case 9600:
    *baud_out = EZO_UART_POSIX_BAUD_9600;
    return EZO_OK;
  case 19200:
    *baud_out = EZO_UART_POSIX_BAUD_19200;
    return EZO_OK;
  case 38400:
    *baud_out = EZO_UART_POSIX_BAUD_38400;
    return EZO_OK;
  case 57600:
    *baud_out = EZO_UART_POSIX_BAUD_57600;
    return EZO_OK;
  case 115200:
    *baud_out = EZO_UART_POSIX_BAUD_115200;
    return EZO_OK;
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }
}

static void ezo_linux_i2c_device_reset(ezo_linux_i2c_device_t *device) {
  if (device == NULL) {
    return;
  }

  memset(device, 0, sizeof(*device));
  device->context.fd = -1;
}

static void ezo_linux_uart_device_reset(ezo_linux_uart_device_t *device) {
  if (device == NULL) {
    return;
  }

  memset(device, 0, sizeof(*device));
  device->serial.fd = -1;
}

static int ezo_linux_i2c_device_is_open(const ezo_linux_i2c_device_t *device) {
  return device != NULL && device->owns_fd != 0 && device->context.fd >= 0 &&
         device->core.transport == ezo_linux_i2c_transport() &&
         device->core.transport_context == (void *)&device->context;
}

static int ezo_linux_uart_device_is_open(const ezo_linux_uart_device_t *device) {
  return device != NULL && device->serial.fd >= 0 && device->serial.owns_fd != 0 &&
         device->serial.has_saved_termios != 0 && device->serial.read_timeout_ms > 0 &&
         device->core.transport == ezo_uart_posix_serial_transport() &&
         device->core.transport_context == (void *)&device->serial;
}

ezo_result_t ezo_linux_i2c_device_open_path(ezo_linux_i2c_device_t *device,
                                            const char *path,
                                            uint8_t address) {
  ezo_linux_i2c_context_t next_context;
  int fd = -1;
  int had_open = 0;
  ezo_result_t result = EZO_OK;

  if (device == NULL || path == NULL || path[0] == '\0') {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  had_open = ezo_linux_i2c_device_is_open(device);

  fd = open(path, O_RDWR);
  if (fd < 0) {
    return EZO_ERR_TRANSPORT;
  }

  result = ezo_linux_i2c_context_init(&next_context, fd);
  if (result != EZO_OK) {
    close(fd);
    return result;
  }

  if (had_open) {
    close(device->context.fd);
  }

  ezo_linux_i2c_device_reset(device);
  device->context = next_context;
  device->owns_fd = 1;

  result = ezo_device_init(&device->core, address, ezo_linux_i2c_transport(), &device->context);
  if (result != EZO_OK) {
    close(device->context.fd);
    ezo_linux_i2c_device_reset(device);
    return result;
  }

  return EZO_OK;
}

ezo_result_t ezo_linux_i2c_device_open_bus(ezo_linux_i2c_device_t *device,
                                           uint32_t bus_index,
                                           uint8_t address) {
  char path[32];
  int written = 0;

  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  written = snprintf(path, sizeof(path), "/dev/i2c-%u", (unsigned int)bus_index);
  if (written <= 0 || (size_t)written >= sizeof(path)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return ezo_linux_i2c_device_open_path(device, path, address);
}

void ezo_linux_i2c_device_close(ezo_linux_i2c_device_t *device) {
  if (device == NULL) {
    return;
  }

  if (ezo_linux_i2c_device_is_open(device)) {
    close(device->context.fd);
  }

  ezo_linux_i2c_device_reset(device);
}

ezo_i2c_device_t *ezo_linux_i2c_device_core(ezo_linux_i2c_device_t *device) {
  if (device == NULL) {
    return NULL;
  }

  return &device->core;
}

ezo_result_t ezo_linux_uart_device_open(ezo_linux_uart_device_t *device,
                                        const char *path,
                                        uint32_t baud_rate,
                                        uint32_t read_timeout_ms) {
  ezo_uart_posix_baud_t baud = EZO_UART_POSIX_BAUD_9600;
  ezo_uart_posix_serial_t next_serial;
  int had_open = 0;
  ezo_result_t result = EZO_OK;

  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  had_open = ezo_linux_uart_device_is_open(device);

  result = ezo_linux_uart_map_baud_rate(baud_rate, &baud);
  if (result != EZO_OK) {
    return result;
  }

  result = ezo_uart_posix_serial_open(&next_serial, path, baud, read_timeout_ms);
  if (result != EZO_OK) {
    return result;
  }

  if (had_open) {
    ezo_uart_posix_serial_close(&device->serial);
  }

  ezo_linux_uart_device_reset(device);
  device->serial = next_serial;

  result = ezo_uart_device_init(&device->core, ezo_uart_posix_serial_transport(), &device->serial);
  if (result != EZO_OK) {
    ezo_uart_posix_serial_close(&device->serial);
    ezo_linux_uart_device_reset(device);
    return result;
  }

  return EZO_OK;
}

void ezo_linux_uart_device_close(ezo_linux_uart_device_t *device) {
  if (device == NULL) {
    return;
  }

  if (ezo_linux_uart_device_is_open(device)) {
    ezo_uart_posix_serial_close(&device->serial);
  }

  ezo_linux_uart_device_reset(device);
}

ezo_uart_device_t *ezo_linux_uart_device_core(ezo_linux_uart_device_t *device) {
  if (device == NULL) {
    return NULL;
  }

  return &device->core;
}
