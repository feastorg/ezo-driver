#include "ezo_ec.h"
#include "ezo_i2c_linux_i2c.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  const char *device_path = "/dev/i2c-1";
  int fd = -1;
  ezo_linux_i2c_context_t transport_context;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_ec_output_config_t output_config;
  ezo_ec_reading_t reading;
  ezo_result_t result = EZO_OK;

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

  result = ezo_device_init(&device, 100, ezo_linux_i2c_transport(), &transport_context);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  result = ezo_ec_send_output_query_i2c(&device, &hint);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  usleep((useconds_t)(hint.wait_ms * 1000U));

  result = ezo_ec_read_output_config_i2c(&device, &output_config);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  result = ezo_ec_send_read_i2c(&device, &hint);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  usleep((useconds_t)(hint.wait_ms * 1000U));

  result = ezo_ec_read_response_i2c(&device, output_config.enabled_mask, &reading);
  close(fd);

  if (result != EZO_OK) {
    fprintf(stderr, "typed EC read failed: %d\n", (int)result);
    return 1;
  }

  if ((reading.present_mask & EZO_EC_OUTPUT_CONDUCTIVITY) != 0u) {
    printf("EC=%.2f uS/cm\n", reading.conductivity_us_cm);
  }
  if ((reading.present_mask & EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS) != 0u) {
    printf("TDS=%.2f ppm\n", reading.total_dissolved_solids_ppm);
  }
  if ((reading.present_mask & EZO_EC_OUTPUT_SALINITY) != 0u) {
    printf("Salinity=%.2f ppt\n", reading.salinity_ppt);
  }
  if ((reading.present_mask & EZO_EC_OUTPUT_SPECIFIC_GRAVITY) != 0u) {
    printf("SpecificGravity=%.3f\n", reading.specific_gravity);
  }

  return 0;
}
