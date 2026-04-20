#include "ezo_linux_device.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  int open_result;
  int close_result;
  int open_call_count;
  int close_call_count;
  int next_fd;
  char last_path[64];
  int last_flags;
  int last_closed_fd;
} test_linux_io_state_t;

typedef struct {
  ezo_result_t open_result;
  int open_call_count;
  int close_call_count;
  uint32_t last_baud_rate;
  uint32_t last_timeout_ms;
  char last_path[64];
} test_uart_state_t;

static test_linux_io_state_t g_i2c_io;
static test_uart_state_t g_uart_state;

static void reset_test_state(void) {
  memset(&g_i2c_io, 0, sizeof(g_i2c_io));
  memset(&g_uart_state, 0, sizeof(g_uart_state));
  g_i2c_io.next_fd = 17;
  g_uart_state.open_result = EZO_OK;
}

int ezo_fake_open(const char *path, int flags, ...) {
  g_i2c_io.open_call_count += 1;
  g_i2c_io.last_flags = flags;
  strncpy(g_i2c_io.last_path, path, sizeof(g_i2c_io.last_path) - 1);
  if (g_i2c_io.open_result < 0) {
    return g_i2c_io.open_result;
  }
  return g_i2c_io.next_fd;
}

int ezo_fake_close(int fd) {
  g_i2c_io.close_call_count += 1;
  g_i2c_io.last_closed_fd = fd;
  return g_i2c_io.close_result;
}

static ezo_result_t test_uart_write_bytes(void *context,
                                          const uint8_t *tx_data,
                                          size_t tx_len) {
  (void)context;
  (void)tx_data;
  (void)tx_len;
  return EZO_OK;
}

static ezo_result_t test_uart_read_bytes(void *context,
                                         uint8_t *rx_data,
                                         size_t rx_len,
                                         size_t *rx_received) {
  (void)context;
  (void)rx_data;
  (void)rx_len;
  if (rx_received != NULL) {
    *rx_received = 0;
  }
  return EZO_OK;
}

static ezo_result_t test_uart_discard_input(void *context) {
  (void)context;
  return EZO_OK;
}

ezo_result_t test_ezo_uart_posix_serial_open(ezo_uart_posix_serial_t *serial,
                                             const char *path,
                                             ezo_uart_posix_baud_t baud,
                                             uint32_t read_timeout_ms) {
  g_uart_state.open_call_count += 1;
  g_uart_state.last_baud_rate = (uint32_t)baud;
  g_uart_state.last_timeout_ms = read_timeout_ms;
  if (path != NULL) {
    strncpy(g_uart_state.last_path, path, sizeof(g_uart_state.last_path) - 1);
  }
  if (serial != NULL) {
    memset(serial, 0, sizeof(*serial));
    serial->fd = 23;
    serial->read_timeout_ms = read_timeout_ms;
    serial->owns_fd = 1;
    serial->has_saved_termios = 1;
  }
  return g_uart_state.open_result;
}

void test_ezo_uart_posix_serial_close(ezo_uart_posix_serial_t *serial) {
  g_uart_state.close_call_count += 1;
  if (serial != NULL) {
    serial->fd = -1;
    serial->read_timeout_ms = 0;
    serial->owns_fd = 0;
    serial->has_saved_termios = 0;
  }
}

const ezo_uart_transport_t *test_ezo_uart_posix_serial_transport(void) {
  static const ezo_uart_transport_t transport = {
      test_uart_write_bytes,
      test_uart_read_bytes,
      test_uart_discard_input,
  };
  return &transport;
}

#define open ezo_fake_open
#define close ezo_fake_close
#define ezo_uart_posix_serial_open test_ezo_uart_posix_serial_open
#define ezo_uart_posix_serial_close test_ezo_uart_posix_serial_close
#define ezo_uart_posix_serial_transport test_ezo_uart_posix_serial_transport
#include "../platform/linux/ezo_linux_device.c"
#undef ezo_uart_posix_serial_transport
#undef ezo_uart_posix_serial_close
#undef ezo_uart_posix_serial_open
#undef close
#undef open

static void test_i2c_open_path_initializes_device_and_close_releases_fd(void) {
  ezo_linux_i2c_device_t device;

  reset_test_state();

  assert(ezo_linux_i2c_device_open_path(&device, "/dev/i2c-7", 0x63) == EZO_OK);
  assert(g_i2c_io.open_call_count == 1);
  assert(strcmp(g_i2c_io.last_path, "/dev/i2c-7") == 0);
  assert(device.context.fd == 17);
  assert(device.owns_fd == 1);
  assert(ezo_linux_i2c_device_core(&device) == &device.core);
  assert(ezo_device_get_address(&device.core) == 0x63);

  ezo_linux_i2c_device_close(&device);
  assert(g_i2c_io.close_call_count == 1);
  assert(g_i2c_io.last_closed_fd == 17);
  assert(device.context.fd == -1);
  assert(device.owns_fd == 0);
}

static void test_i2c_open_bus_formats_expected_path(void) {
  ezo_linux_i2c_device_t device;

  reset_test_state();

  assert(ezo_linux_i2c_device_open_bus(&device, 3, 0x64) == EZO_OK);
  assert(strcmp(g_i2c_io.last_path, "/dev/i2c-3") == 0);

  ezo_linux_i2c_device_close(&device);
}

static void test_i2c_reopen_closes_previous_fd_after_success(void) {
  ezo_linux_i2c_device_t device;

  reset_test_state();

  assert(ezo_linux_i2c_device_open_path(&device, "/dev/i2c-7", 0x63) == EZO_OK);
  g_i2c_io.next_fd = 29;
  assert(ezo_linux_i2c_device_open_path(&device, "/dev/i2c-8", 0x64) == EZO_OK);
  assert(g_i2c_io.open_call_count == 2);
  assert(g_i2c_io.close_call_count == 1);
  assert(g_i2c_io.last_closed_fd == 17);
  assert(strcmp(g_i2c_io.last_path, "/dev/i2c-8") == 0);
  assert(device.context.fd == 29);
  assert(device.owns_fd == 1);
  assert(ezo_device_get_address(&device.core) == 0x64);

  ezo_linux_i2c_device_close(&device);
  assert(g_i2c_io.close_call_count == 2);
  assert(g_i2c_io.last_closed_fd == 29);
}

static void test_i2c_reopen_failure_keeps_existing_fd_open(void) {
  ezo_linux_i2c_device_t device;

  reset_test_state();

  assert(ezo_linux_i2c_device_open_path(&device, "/dev/i2c-7", 0x63) == EZO_OK);
  g_i2c_io.open_result = -1;
  assert(ezo_linux_i2c_device_open_path(&device, "/dev/i2c-8", 0x64) == EZO_ERR_TRANSPORT);
  assert(g_i2c_io.open_call_count == 2);
  assert(g_i2c_io.close_call_count == 0);
  assert(device.context.fd == 17);
  assert(device.owns_fd == 1);
  assert(ezo_device_get_address(&device.core) == 0x63);

  ezo_linux_i2c_device_close(&device);
  assert(g_i2c_io.close_call_count == 1);
  assert(g_i2c_io.last_closed_fd == 17);
}

static void test_uart_open_initializes_device_and_close_calls_serial_close(void) {
  ezo_linux_uart_device_t device;

  reset_test_state();

  assert(ezo_linux_uart_device_open(&device, "/dev/ttyS0", 9600, 250) == EZO_OK);
  assert(g_uart_state.open_call_count == 1);
  assert(strcmp(g_uart_state.last_path, "/dev/ttyS0") == 0);
  assert(g_uart_state.last_timeout_ms == 250);
  assert(device.serial.fd == 23);
  assert(ezo_linux_uart_device_core(&device) == &device.core);

  ezo_linux_uart_device_close(&device);
  assert(g_uart_state.close_call_count == 1);
  assert(device.serial.fd == -1);
}

static void test_uart_reopen_closes_previous_session_after_success(void) {
  ezo_linux_uart_device_t device;

  reset_test_state();

  assert(ezo_linux_uart_device_open(&device, "/dev/ttyS0", 9600, 250) == EZO_OK);
  assert(ezo_linux_uart_device_open(&device, "/dev/ttyAMA0", 19200, 500) == EZO_OK);
  assert(g_uart_state.open_call_count == 2);
  assert(g_uart_state.close_call_count == 1);
  assert(strcmp(g_uart_state.last_path, "/dev/ttyAMA0") == 0);
  assert(g_uart_state.last_timeout_ms == 500);
  assert(device.serial.fd == 23);

  ezo_linux_uart_device_close(&device);
  assert(g_uart_state.close_call_count == 2);
}

static void test_uart_reopen_failure_keeps_existing_session_open(void) {
  ezo_linux_uart_device_t device;

  reset_test_state();

  assert(ezo_linux_uart_device_open(&device, "/dev/ttyS0", 9600, 250) == EZO_OK);
  g_uart_state.open_result = EZO_ERR_TRANSPORT;
  assert(ezo_linux_uart_device_open(&device, "/dev/ttyAMA0", 9600, 250) == EZO_ERR_TRANSPORT);
  assert(g_uart_state.open_call_count == 2);
  assert(g_uart_state.close_call_count == 0);
  assert(device.serial.fd == 23);

  ezo_linux_uart_device_close(&device);
  assert(g_uart_state.close_call_count == 1);
}

static void test_open_helpers_reject_invalid_arguments(void) {
  ezo_linux_i2c_device_t i2c_device;
  ezo_linux_uart_device_t uart_device;

  reset_test_state();

  assert(ezo_linux_i2c_device_open_path(NULL, "/dev/i2c-1", 0x63) == EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_linux_i2c_device_open_path(&i2c_device, NULL, 0x63) == EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_linux_i2c_device_open_path(&i2c_device, "", 0x63) == EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_linux_uart_device_open(NULL, "/dev/ttyS0", 9600, 100) == EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_linux_uart_device_open(&uart_device, "/dev/ttyS0", 12345, 100) ==
         EZO_ERR_INVALID_ARGUMENT);
}

int main(void) {
  test_i2c_open_path_initializes_device_and_close_releases_fd();
  test_i2c_open_bus_formats_expected_path();
  test_i2c_reopen_closes_previous_fd_after_success();
  test_i2c_reopen_failure_keeps_existing_fd_open();
  test_uart_open_initializes_device_and_close_calls_serial_close();
  test_uart_reopen_closes_previous_session_after_success();
  test_uart_reopen_failure_keeps_existing_session_open();
  test_open_helpers_reject_invalid_arguments();
  return 0;
}
