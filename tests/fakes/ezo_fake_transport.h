#ifndef TESTS_FAKES_EZO_FAKE_TRANSPORT_H
#define TESTS_FAKES_EZO_FAKE_TRANSPORT_H

#include "ezo_i2c/ezo_i2c.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EZO_FAKE_TRANSPORT_MAX_TX 128
#define EZO_FAKE_TRANSPORT_MAX_RX 128

typedef struct {
  ezo_result_t callback_result;
  uint8_t expected_address;
  int enforce_expected_address;
  uint8_t response_bytes[EZO_FAKE_TRANSPORT_MAX_RX];
  size_t response_len;
  size_t call_count;
  uint8_t last_tx_bytes[EZO_FAKE_TRANSPORT_MAX_TX];
  size_t last_tx_len;
  size_t last_rx_len_requested;
} ezo_fake_transport_t;

void ezo_fake_transport_init(ezo_fake_transport_t *fake);
void ezo_fake_transport_set_response(ezo_fake_transport_t *fake,
                                     const uint8_t *response_bytes,
                                     size_t response_len);
const ezo_i2c_transport_t *ezo_fake_transport_vtable(void);

#ifdef __cplusplus
}
#endif

#endif
