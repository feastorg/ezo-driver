#include "ezo_control.h"
#include "ezo_i2c_linux_i2c.h"
#include "ezo_product.h"

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
  ezo_device_info_t info;
  const ezo_product_metadata_t *metadata = NULL;
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

  result = ezo_control_send_info_query_i2c(&device, EZO_PRODUCT_UNKNOWN, &hint);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  usleep((useconds_t)(hint.wait_ms * 1000U));

  result = ezo_control_read_info_i2c(&device, &info);
  close(fd);
  if (result != EZO_OK) {
    fprintf(stderr, "info query failed: %d\n", (int)result);
    return 1;
  }

  metadata = ezo_product_get_metadata(info.product_id);

  printf("product_code=%s firmware=%s\n", info.product_code, info.firmware_version);
  if (metadata != NULL) {
    printf("product_id=%d tier=%d default_i2c=%u\n",
           (int)metadata->product_id,
           (int)metadata->support_tier,
           (unsigned)metadata->default_i2c_address);
  }

  return 0;
}
