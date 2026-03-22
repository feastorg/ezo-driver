#include "example_i2c.h"

#include <fcntl.h>
#include <unistd.h>

ezo_result_t ezo_example_open_i2c(const char *device_path,
                                  uint8_t address,
                                  ezo_example_i2c_session_t *session_out) {
  ezo_result_t result = EZO_OK;

  if (device_path == NULL || session_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  session_out->fd = open(device_path, O_RDWR);
  if (session_out->fd < 0) {
    return EZO_ERR_TRANSPORT;
  }

  result = ezo_linux_i2c_context_init(&session_out->transport_context, session_out->fd);
  if (result != EZO_OK) {
    close(session_out->fd);
    session_out->fd = -1;
    return result;
  }

  result = ezo_device_init(&session_out->device,
                           address,
                           ezo_linux_i2c_transport(),
                           &session_out->transport_context);
  if (result != EZO_OK) {
    close(session_out->fd);
    session_out->fd = -1;
    return result;
  }

  return EZO_OK;
}

void ezo_example_close_i2c(ezo_example_i2c_session_t *session) {
  if (session == NULL || session->fd < 0) {
    return;
  }

  close(session->fd);
  session->fd = -1;
}
