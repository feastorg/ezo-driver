#include "ezo_i2c/ezo_i2c.h"
#include "tests/fakes/ezo_fake_transport.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static void test_send_command_records_bytes(void) {
  ezo_fake_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_result_t result;

  ezo_fake_transport_init(&fake);
  result = ezo_device_init(&device, 99, ezo_fake_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_send_command(&device, "name,?", EZO_COMMAND_GENERIC, &hint);
  assert(result == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.call_count == 1);
  assert(fake.last_tx_len == strlen("name,?"));
  assert(memcmp(fake.last_tx_bytes, "name,?", strlen("name,?")) == 0);
}

static void test_read_response_success_and_parse(void) {
  static const uint8_t response[] = {1, '7', '.', '1', '2', 0};
  ezo_fake_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status;
  ezo_result_t result;
  char buffer[16];
  size_t response_len = 0;
  double value = 0.0;

  ezo_fake_transport_init(&fake);
  ezo_fake_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_OK);
  assert(status == EZO_STATUS_SUCCESS);
  assert(response_len == 4);
  assert(memcmp(buffer, "7.12", 4) == 0);

  result = ezo_parse_double(buffer, response_len, &value);
  assert(result == EZO_OK);
  assert(value > 7.11 && value < 7.13);
}

static void test_read_response_unknown_status_is_protocol_error(void) {
  static const uint8_t response[] = {42, 'x', 0};
  ezo_fake_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status;
  ezo_result_t result;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_transport_init(&fake);
  ezo_fake_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_ERR_PROTOCOL);
  assert(status == EZO_STATUS_UNKNOWN);
  assert(response_len == 0);
}

static void test_transport_failure_is_not_device_status(void) {
  ezo_fake_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status;
  ezo_result_t result;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_transport_init(&fake);
  fake.callback_result = EZO_ERR_TRANSPORT;
  result = ezo_device_init(&device, 100, ezo_fake_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_ERR_TRANSPORT);
  assert(status == EZO_STATUS_UNKNOWN);
  assert(response_len == 0);
}

int main(void) {
  test_send_command_records_bytes();
  test_read_response_success_and_parse();
  test_read_response_unknown_status_is_protocol_error();
  test_transport_failure_is_not_device_status();
  return 0;
}
