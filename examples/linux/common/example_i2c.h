#ifndef EZO_EXAMPLE_I2C_H
#define EZO_EXAMPLE_I2C_H

#include "ezo_i2c_linux_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int fd;
  ezo_linux_i2c_context_t transport_context;
  ezo_i2c_device_t device;
} ezo_example_i2c_session_t;

ezo_result_t ezo_example_open_i2c(const char *device_path,
                                  uint8_t address,
                                  ezo_example_i2c_session_t *session_out);

void ezo_example_close_i2c(ezo_example_i2c_session_t *session);

#ifdef __cplusplus
}
#endif

#endif
