#include "ezo_i2c_linux_i2c.h"
#include "ezo_i2c.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  const char *device_path = "/dev/i2c-1";
  uint8_t address = 99;
  int fd = -1;
  ezo_linux_i2c_context_t transport_context;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  char response[32];
  size_t response_len = 0;
  ezo_result_t result;

  if (argc > 1) {
    device_path = argv[1];
  }

  fd = open(device_path, O_RDWR);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  result = ezo_linux_i2c_context_init(&transport_context, fd);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  result = ezo_device_init(&device, address, ezo_linux_i2c_transport(), &transport_context);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  result = ezo_send_command(&device, "name,?", EZO_COMMAND_GENERIC, &hint);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  usleep((useconds_t)(hint.wait_ms * 1000U));

  result = ezo_read_response(&device, response, sizeof(response), &response_len, &status);
  close(fd);

  if (result != EZO_OK) {
    fprintf(stderr, "read failed: %d\n", (int)result);
    return 1;
  }

  printf("status=%d response=%s\n", (int)status, response);
  return 0;
}
