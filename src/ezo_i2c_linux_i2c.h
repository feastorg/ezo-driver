#ifndef EZO_I2C_LINUX_I2C_H
#define EZO_I2C_LINUX_I2C_H

#include "ezo_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int fd;
  /* errno from the last failed call on this context, 0 after a successful one.
   * Read it after a call returns EZO_ERR_TRANSPORT. The transport records the
   * errno of a failed ioctl(I2C_SLAVE), write or read, and EIO for a short
   * write, which sets no errno. ezo_linux_i2c_device_open_path() records the
   * errno of a failed open() here; context_init() clears it. */
  int last_errno;
} ezo_linux_i2c_context_t;

ezo_result_t ezo_linux_i2c_context_init(ezo_linux_i2c_context_t *context, int fd);
const ezo_i2c_transport_t *ezo_linux_i2c_transport(void);

#ifdef __cplusplus
}
#endif

#endif
