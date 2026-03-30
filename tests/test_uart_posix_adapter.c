#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _XOPEN_SOURCE 600

#include "ezo_uart.h"
#include "ezo_uart_posix_serial.h"

#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

typedef struct {
  int master_fd;
  char slave_path[128];
  struct termios original_termios;
} ezo_test_pty_t;

static void assert_termios_equal(const struct termios *expected,
                                 const struct termios *actual) {
  assert(expected != NULL);
  assert(actual != NULL);
  assert(expected->c_iflag == actual->c_iflag);
  assert(expected->c_oflag == actual->c_oflag);
  assert(expected->c_cflag == actual->c_cflag);
  assert(expected->c_lflag == actual->c_lflag);
  assert(memcmp(expected->c_cc, actual->c_cc, NCCS) == 0);
  assert(cfgetispeed(actual) == cfgetispeed(expected));
  assert(cfgetospeed(actual) == cfgetospeed(expected));
}

static void ezo_test_pty_init(ezo_test_pty_t *pty) {
  int probe_fd = -1;

  assert(pty != NULL);

  memset(pty, 0, sizeof(*pty));
  pty->master_fd = posix_openpt(O_RDWR | O_NOCTTY);
  assert(pty->master_fd >= 0);
  assert(grantpt(pty->master_fd) == 0);
  assert(unlockpt(pty->master_fd) == 0);
  assert(ptsname_r(pty->master_fd, pty->slave_path, sizeof(pty->slave_path)) == 0);

  probe_fd = open(pty->slave_path, O_RDWR | O_NOCTTY);
  assert(probe_fd >= 0);
  assert(tcgetattr(probe_fd, &pty->original_termios) == 0);
  close(probe_fd);
}

static void ezo_test_pty_close(ezo_test_pty_t *pty) {
  if (pty != NULL && pty->master_fd >= 0) {
    close(pty->master_fd);
    pty->master_fd = -1;
  }
}

static void test_posix_uart_open_rejects_invalid_arguments(void) {
  ezo_uart_posix_serial_t serial;

  assert(ezo_uart_posix_serial_open(NULL, "/dev/null", EZO_UART_POSIX_BAUD_9600, 100) ==
         EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_uart_posix_serial_open(&serial, NULL, EZO_UART_POSIX_BAUD_9600, 100) ==
         EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_uart_posix_serial_open(&serial, "", EZO_UART_POSIX_BAUD_9600, 100) ==
         EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_uart_posix_serial_open(&serial, "/dev/null", EZO_UART_POSIX_BAUD_9600, 0) ==
         EZO_ERR_INVALID_ARGUMENT);
}

static void test_posix_uart_send_command_writes_bytes(void) {
  ezo_test_pty_t pty;
  ezo_uart_posix_serial_t serial;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  uint8_t rx_buffer[8];
  ssize_t received = 0;

  ezo_test_pty_init(&pty);
  assert(ezo_uart_posix_serial_open(&serial, pty.slave_path, EZO_UART_POSIX_BAUD_9600, 100) ==
         EZO_OK);
  assert(ezo_uart_device_init(&device, ezo_uart_posix_serial_transport(), &serial) == EZO_OK);
  assert(ezo_uart_send_command(&device, "i", EZO_COMMAND_GENERIC, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);

  received = read(pty.master_fd, rx_buffer, sizeof(rx_buffer));
  assert(received == 2);
  assert(memcmp(rx_buffer, "i\r", 2) == 0);

  ezo_uart_posix_serial_close(&serial);
  ezo_test_pty_close(&pty);
}

static void test_posix_uart_read_line_round_trips_through_core(void) {
  static const uint8_t response[] = {'1', '2', '.', '3', '4', '\r'};
  ezo_test_pty_t pty;
  ezo_uart_posix_serial_t serial;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[16];
  size_t response_len = 0;

  ezo_test_pty_init(&pty);
  assert(ezo_uart_posix_serial_open(&serial, pty.slave_path, EZO_UART_POSIX_BAUD_9600, 100) ==
         EZO_OK);
  assert(ezo_uart_device_init(&device, ezo_uart_posix_serial_transport(), &serial) == EZO_OK);
  assert(write(pty.master_fd, response, sizeof(response)) == (ssize_t)sizeof(response));

  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) ==
         EZO_OK);
  assert(kind == EZO_UART_RESPONSE_DATA);
  assert(response_len == 5);
  assert(memcmp(buffer, "12.34", 5) == 0);

  ezo_uart_posix_serial_close(&serial);
  ezo_test_pty_close(&pty);
}

static void test_posix_uart_discard_input_flushes_pending_bytes(void) {
  static const uint8_t response[] = {'j', 'u', 'n', 'k', '\r'};
  ezo_test_pty_t pty;
  ezo_uart_posix_serial_t serial;
  ezo_uart_device_t device;
  uint8_t byte = 0;
  size_t received = 1;

  ezo_test_pty_init(&pty);
  assert(ezo_uart_posix_serial_open(&serial, pty.slave_path, EZO_UART_POSIX_BAUD_9600, 100) ==
         EZO_OK);
  assert(ezo_uart_device_init(&device, ezo_uart_posix_serial_transport(), &serial) == EZO_OK);
  assert(write(pty.master_fd, response, sizeof(response)) == (ssize_t)sizeof(response));

  assert(ezo_uart_discard_input(&device) == EZO_OK);
  assert(ezo_uart_posix_serial_transport()->read_bytes(&serial, &byte, 1, &received) == EZO_OK);
  assert(received == 0);

  ezo_uart_posix_serial_close(&serial);
  ezo_test_pty_close(&pty);
}

static void test_posix_uart_close_restores_termios(void) {
  ezo_test_pty_t pty;
  ezo_uart_posix_serial_t serial;
  struct termios restored_termios;
  int fd = -1;

  ezo_test_pty_init(&pty);
  assert(ezo_uart_posix_serial_open(&serial, pty.slave_path, EZO_UART_POSIX_BAUD_9600, 100) ==
         EZO_OK);
  ezo_uart_posix_serial_close(&serial);

  fd = open(pty.slave_path, O_RDWR | O_NOCTTY);
  assert(fd >= 0);
  assert(tcgetattr(fd, &restored_termios) == 0);
  assert_termios_equal(&pty.original_termios, &restored_termios);
  close(fd);

  ezo_test_pty_close(&pty);
}

int main(void) {
  test_posix_uart_open_rejects_invalid_arguments();
  test_posix_uart_send_command_writes_bytes();
  test_posix_uart_read_line_round_trips_through_core();
  test_posix_uart_discard_input_flushes_pending_bytes();
  test_posix_uart_close_restores_termios();
  return 0;
}
