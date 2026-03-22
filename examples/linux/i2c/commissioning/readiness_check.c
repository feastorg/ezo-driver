/*
Purpose: inspect an I2C device and print product-specific readiness signals.
Defaults: /dev/i2c-1 and address 99.
Assumptions: the device is already in I2C mode and the address is known.
Next: jump to ../typed/read_<product>.c for a simple read or ../advanced/ for stateful flows.
*/

#include "example_base.h"
#include "example_i2c.h"
#include "example_product_i2c.h"

#include <stdio.h>

int main(int argc, char **argv) {
  ezo_example_i2c_options_t options;
  ezo_example_i2c_session_t session;
  ezo_device_info_t info;
  const ezo_product_metadata_t *metadata = NULL;
  ezo_result_t result = EZO_OK;
  int next_arg = 0;

  if (!ezo_example_parse_i2c_options(argc, argv, 99U, &options, &next_arg)) {
    fprintf(stderr, "usage: %s [device_path] [address]\n", argv[0]);
    return 1;
  }

  result = ezo_example_open_i2c(options.device_path, options.address, &session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_i2c", result);
  }

  result = ezo_example_query_info_i2c(&session.device, &info);
  if (result != EZO_OK) {
    ezo_example_close_i2c(&session);
    return ezo_example_print_error("query_info", result);
  }

  metadata = ezo_product_get_metadata(info.product_id);

  printf("transport=i2c\n");
  printf("device_path=%s\n", options.device_path);
  printf("address=%u\n", (unsigned)options.address);
  printf("product=%s\n", ezo_example_product_name(info.product_id));
  ezo_example_print_device_info(&info);
  ezo_example_print_product_metadata(metadata);

  result = ezo_example_print_shared_control_i2c(&session.device, info.product_id);
  if (result == EZO_OK) {
    result = ezo_example_print_product_readiness_i2c(&session.device, info.product_id);
  }

  ezo_example_close_i2c(&session);
  if (result != EZO_OK) {
    return ezo_example_print_error("readiness_check", result);
  }

  return 0;
}
