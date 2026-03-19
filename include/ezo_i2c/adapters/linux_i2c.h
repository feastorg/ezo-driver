#ifndef EZO_I2C_ADAPTERS_LINUX_I2C_H
#define EZO_I2C_ADAPTERS_LINUX_I2C_H

#include "ezo_i2c/ezo_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int fd;
} ezo_linux_i2c_context_t;

ezo_result_t ezo_linux_i2c_context_init(ezo_linux_i2c_context_t *context, int fd);
const ezo_i2c_transport_t *ezo_linux_i2c_transport(void);

#ifdef __cplusplus
}
#endif

#endif
