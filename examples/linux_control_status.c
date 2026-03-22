#include "ezo_control.h"
#include "ezo_i2c_linux_i2c.h"

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
  ezo_control_name_t name;
  ezo_control_status_t status;
  ezo_control_led_status_t led;
  ezo_result_t result = EZO_OK;

  if (argc > 1) {
    device_path = argv[1];
  }

  if (argc > 2) {
    address = (uint8_t)atoi(argv[2]);
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

  result = ezo_control_send_name_query_i2c(&device, EZO_PRODUCT_UNKNOWN, &hint);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }
  usleep((useconds_t)(hint.wait_ms * 1000U));
  result = ezo_control_read_name_i2c(&device, &name);
  if (result != EZO_OK) {
    close(fd);
    fprintf(stderr, "name query failed: %d\n", (int)result);
    return 1;
  }

  result = ezo_control_send_status_query_i2c(&device, EZO_PRODUCT_UNKNOWN, &hint);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }
  usleep((useconds_t)(hint.wait_ms * 1000U));
  result = ezo_control_read_status_i2c(&device, &status);
  if (result != EZO_OK) {
    close(fd);
    fprintf(stderr, "status query failed: %d\n", (int)result);
    return 1;
  }

  result = ezo_control_send_led_query_i2c(&device, EZO_PRODUCT_UNKNOWN, &hint);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }
  usleep((useconds_t)(hint.wait_ms * 1000U));
  result = ezo_control_read_led_i2c(&device, &led);
  close(fd);
  if (result != EZO_OK) {
    fprintf(stderr, "led query failed: %d\n", (int)result);
    return 1;
  }

  printf("name=%s\n", name.name);
  printf("restart_code=%c supply_voltage=%.3f led=%u\n",
         status.restart_code,
         status.supply_voltage,
         (unsigned)led.enabled);
  return 0;
}
