#include "ezo_i2c/linux_i2c.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

typedef struct {
  int ioctl_result;
  ssize_t write_result;
  ssize_t read_result;
  uint8_t read_bytes[32];
  size_t read_bytes_len;
  int ioctl_call_count;
  int write_call_count;
  int read_call_count;
  int last_fd;
  int last_address;
  unsigned long last_ioctl_request;
  uint8_t last_write_bytes[32];
  size_t last_write_len;
} test_linux_syscalls_t;

static test_linux_syscalls_t g_test_linux_syscalls;

static void reset_test_linux_syscalls(void) {
  memset(&g_test_linux_syscalls, 0, sizeof(g_test_linux_syscalls));
  g_test_linux_syscalls.ioctl_result = 0;
}

int test_ioctl(int fd, unsigned long request, ...) {
  va_list args;
  int address = 0;

  va_start(args, request);
  address = va_arg(args, int);
  va_end(args);

  g_test_linux_syscalls.ioctl_call_count += 1;
  g_test_linux_syscalls.last_fd = fd;
  g_test_linux_syscalls.last_address = address;
  g_test_linux_syscalls.last_ioctl_request = request;

  return g_test_linux_syscalls.ioctl_result;
}

ssize_t test_write(int fd, const void *buffer, size_t len) {
  g_test_linux_syscalls.write_call_count += 1;
  g_test_linux_syscalls.last_fd = fd;
  g_test_linux_syscalls.last_write_len = len;

  if (buffer != NULL && len <= sizeof(g_test_linux_syscalls.last_write_bytes)) {
    memcpy(g_test_linux_syscalls.last_write_bytes, buffer, len);
  }

  return g_test_linux_syscalls.write_result;
}

ssize_t test_read(int fd, void *buffer, size_t len) {
  size_t to_copy = g_test_linux_syscalls.read_bytes_len;

  g_test_linux_syscalls.read_call_count += 1;
  g_test_linux_syscalls.last_fd = fd;

  if (g_test_linux_syscalls.read_result < 0) {
    return g_test_linux_syscalls.read_result;
  }

  if ((size_t)g_test_linux_syscalls.read_result < to_copy) {
    to_copy = (size_t)g_test_linux_syscalls.read_result;
  }

  if (len < to_copy) {
    to_copy = len;
  }

  if (buffer != NULL && to_copy > 0) {
    memcpy(buffer, g_test_linux_syscalls.read_bytes, to_copy);
  }

  return g_test_linux_syscalls.read_result;
}

#define ioctl test_ioctl
#define write test_write
#define read test_read
#include "../platform/linux/ezo_linux_i2c.c"
#undef read
#undef write
#undef ioctl

static void test_context_init_rejects_invalid_arguments(void) {
  ezo_linux_i2c_context_t context;

  assert(ezo_linux_i2c_context_init(NULL, 3) == EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_linux_i2c_context_init(&context, -1) == EZO_ERR_INVALID_ARGUMENT);
}

static void test_transport_rejects_invalid_arguments(void) {
  ezo_linux_i2c_context_t context;
  const ezo_i2c_transport_t *transport = ezo_linux_i2c_transport();
  uint8_t byte = 0;
  size_t rx_received = 99;

  assert(ezo_linux_i2c_context_init(&context, 7) == EZO_OK);
  assert(transport->write_then_read(NULL, 100, &byte, 1, NULL, 0, &rx_received) ==
         EZO_ERR_INVALID_ARGUMENT);
  assert(transport->write_then_read(&context, 100, NULL, 1, NULL, 0, &rx_received) ==
         EZO_ERR_INVALID_ARGUMENT);
  assert(transport->write_then_read(&context, 100, NULL, 0, NULL, 1, &rx_received) ==
         EZO_ERR_INVALID_ARGUMENT);
}

static void test_transport_returns_transport_error_when_ioctl_fails(void) {
  ezo_linux_i2c_context_t context;
  const ezo_i2c_transport_t *transport = ezo_linux_i2c_transport();
  uint8_t tx = 'r';
  size_t rx_received = 99;

  reset_test_linux_syscalls();
  g_test_linux_syscalls.ioctl_result = -1;

  assert(ezo_linux_i2c_context_init(&context, 5) == EZO_OK);
  assert(transport->write_then_read(&context, 100, &tx, 1, NULL, 0, &rx_received) ==
         EZO_ERR_TRANSPORT);
  assert(rx_received == 0);
  assert(g_test_linux_syscalls.ioctl_call_count == 1);
  assert(g_test_linux_syscalls.last_ioctl_request == I2C_SLAVE);
  assert(g_test_linux_syscalls.last_address == 100);
  assert(g_test_linux_syscalls.write_call_count == 0);
  assert(g_test_linux_syscalls.read_call_count == 0);
}

static void test_transport_returns_transport_error_on_short_write(void) {
  ezo_linux_i2c_context_t context;
  const ezo_i2c_transport_t *transport = ezo_linux_i2c_transport();
  const uint8_t tx[] = {'n', 'a', 'm', 'e'};
  size_t rx_received = 99;

  reset_test_linux_syscalls();
  g_test_linux_syscalls.write_result = 3;

  assert(ezo_linux_i2c_context_init(&context, 6) == EZO_OK);
  assert(transport->write_then_read(&context,
                                    101,
                                    tx,
                                    sizeof(tx),
                                    NULL,
                                    0,
                                    &rx_received) == EZO_ERR_TRANSPORT);
  assert(rx_received == 0);
  assert(g_test_linux_syscalls.ioctl_call_count == 1);
  assert(g_test_linux_syscalls.last_ioctl_request == I2C_SLAVE);
  assert(g_test_linux_syscalls.last_address == 101);
  assert(g_test_linux_syscalls.write_call_count == 1);
  assert(g_test_linux_syscalls.read_call_count == 0);
  assert(g_test_linux_syscalls.last_write_len == sizeof(tx));
  assert(memcmp(g_test_linux_syscalls.last_write_bytes, tx, sizeof(tx)) == 0);
}

static void test_transport_supports_write_only_transactions(void) {
  ezo_linux_i2c_context_t context;
  const ezo_i2c_transport_t *transport = ezo_linux_i2c_transport();
  const uint8_t tx[] = {'r'};
  size_t rx_received = 99;

  reset_test_linux_syscalls();
  g_test_linux_syscalls.write_result = (ssize_t)sizeof(tx);

  assert(ezo_linux_i2c_context_init(&context, 8) == EZO_OK);
  assert(transport->write_then_read(&context,
                                    102,
                                    tx,
                                    sizeof(tx),
                                    NULL,
                                    0,
                                    &rx_received) == EZO_OK);
  assert(rx_received == 0);
  assert(g_test_linux_syscalls.ioctl_call_count == 1);
  assert(g_test_linux_syscalls.last_ioctl_request == I2C_SLAVE);
  assert(g_test_linux_syscalls.last_address == 102);
  assert(g_test_linux_syscalls.write_call_count == 1);
  assert(g_test_linux_syscalls.read_call_count == 0);
}

static void test_transport_returns_transport_error_on_read_failure(void) {
  ezo_linux_i2c_context_t context;
  const ezo_i2c_transport_t *transport = ezo_linux_i2c_transport();
  uint8_t rx[8];
  size_t rx_received = 99;

  reset_test_linux_syscalls();
  g_test_linux_syscalls.read_result = -1;

  assert(ezo_linux_i2c_context_init(&context, 9) == EZO_OK);
  assert(transport->write_then_read(&context, 103, NULL, 0, rx, sizeof(rx), &rx_received) ==
         EZO_ERR_TRANSPORT);
  assert(rx_received == 0);
  assert(g_test_linux_syscalls.ioctl_call_count == 1);
  assert(g_test_linux_syscalls.last_ioctl_request == I2C_SLAVE);
  assert(g_test_linux_syscalls.last_address == 103);
  assert(g_test_linux_syscalls.write_call_count == 0);
  assert(g_test_linux_syscalls.read_call_count == 1);
}

static void test_transport_reads_bytes_and_reports_actual_count(void) {
  ezo_linux_i2c_context_t context;
  const ezo_i2c_transport_t *transport = ezo_linux_i2c_transport();
  const uint8_t tx[] = {'r'};
  uint8_t rx[8] = {0};
  size_t rx_received = 0;

  reset_test_linux_syscalls();
  g_test_linux_syscalls.write_result = (ssize_t)sizeof(tx);
  g_test_linux_syscalls.read_result = 3;
  g_test_linux_syscalls.read_bytes[0] = 1;
  g_test_linux_syscalls.read_bytes[1] = '7';
  g_test_linux_syscalls.read_bytes[2] = 0;
  g_test_linux_syscalls.read_bytes_len = 3;

  assert(ezo_linux_i2c_context_init(&context, 10) == EZO_OK);
  assert(transport->write_then_read(&context,
                                    104,
                                    tx,
                                    sizeof(tx),
                                    rx,
                                    sizeof(rx),
                                    &rx_received) == EZO_OK);
  assert(rx_received == 3);
  assert(g_test_linux_syscalls.ioctl_call_count == 1);
  assert(g_test_linux_syscalls.last_ioctl_request == I2C_SLAVE);
  assert(g_test_linux_syscalls.last_address == 104);
  assert(g_test_linux_syscalls.write_call_count == 1);
  assert(g_test_linux_syscalls.read_call_count == 1);
  assert(rx[0] == 1);
  assert(rx[1] == '7');
  assert(rx[2] == 0);
}

int main(void) {
  test_context_init_rejects_invalid_arguments();
  test_transport_rejects_invalid_arguments();
  test_transport_returns_transport_error_when_ioctl_fails();
  test_transport_returns_transport_error_on_short_write();
  test_transport_supports_write_only_transactions();
  test_transport_returns_transport_error_on_read_failure();
  test_transport_reads_bytes_and_reports_actual_count();
  return 0;
}
