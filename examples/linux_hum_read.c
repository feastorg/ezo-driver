#include "ezo_hum.h"
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
  ezo_hum_output_config_t output_config;
  ezo_hum_reading_t reading;
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

  result = ezo_device_init(&device, 111, ezo_linux_i2c_transport(), &transport_context);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  result = ezo_hum_send_output_query_i2c(&device, &hint);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  usleep((useconds_t)(hint.wait_ms * 1000U));

  result = ezo_hum_read_output_config_i2c(&device, &output_config);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  result = ezo_hum_send_read_i2c(&device, &hint);
  if (result != EZO_OK) {
    close(fd);
    return 1;
  }

  usleep((useconds_t)(hint.wait_ms * 1000U));

  result = ezo_hum_read_response_i2c(&device, output_config.enabled_mask, &reading);
  close(fd);

  if (result != EZO_OK) {
    fprintf(stderr, "typed HUM read failed: %d\n", (int)result);
    return 1;
  }

  if ((reading.present_mask & EZO_HUM_OUTPUT_HUMIDITY) != 0u) {
    printf("Humidity=%.2f %%RH\n", reading.relative_humidity_percent);
  }
  if ((reading.present_mask & EZO_HUM_OUTPUT_AIR_TEMPERATURE) != 0u) {
    printf("AirTemperature=%.2f C\n", reading.air_temperature_c);
  }
  if ((reading.present_mask & EZO_HUM_OUTPUT_DEW_POINT) != 0u) {
    printf("DewPoint=%.2f C\n", reading.dew_point_c);
  }

  return 0;
}
