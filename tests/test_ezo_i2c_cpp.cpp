#include "ezo_i2c.hpp"
#include "tests/fakes/ezo_fake_transport.h"

#include <cassert>
#include <cstddef>
#include <cstring>

static void test_cpp_wrapper_send_read_and_parse(void) {
  static const uint8_t response[] = {1, '1', '2', '.', '3', '4', 0};
  ezo_fake_transport_t fake;
  ezo_i2c::Device device;
  ezo_i2c::TimingHint hint;
  ezo_i2c::DeviceStatus status = EZO_STATUS_UNKNOWN;
  char buffer[16];
  std::size_t response_len = 0;
  double value = 0.0;

  ezo_fake_transport_init(&fake);
  ezo_fake_transport_set_response(&fake, response, sizeof(response));

  assert(device.init(100, ezo_fake_transport_vtable(), &fake) == EZO_OK);
  assert(device.send_read(&hint) == EZO_OK);
  assert(hint.wait_ms == 1000);
  assert(fake.last_tx_len == 1);
  assert(fake.last_tx_bytes[0] == 'r');

  assert(device.read_response(buffer, sizeof(buffer), &response_len, &status) == EZO_OK);
  assert(status == EZO_STATUS_SUCCESS);
  assert(response_len == 5);
  assert(std::memcmp(buffer, "12.34", response_len) == 0);

  assert(device.parse_double(buffer, response_len, &value) == EZO_OK);
  assert(value > 12.33 && value < 12.35);
  assert(device.last_status() == EZO_STATUS_SUCCESS);
}

static void test_cpp_wrapper_reads_raw_response(void) {
  static const uint8_t response[] = {1, 'O', 'K', 0};
  ezo_fake_transport_t fake;
  ezo_i2c::Device device;
  ezo_i2c::DeviceStatus status = EZO_STATUS_UNKNOWN;
  uint8_t buffer[8];
  size_t response_len = 0;

  ezo_fake_transport_init(&fake);
  ezo_fake_transport_set_response(&fake, response, sizeof(response));

  assert(device.init(100, ezo_fake_transport_vtable(), &fake) == EZO_OK);
  assert(device.read_response_raw(buffer, sizeof(buffer), &response_len, &status) == EZO_OK);
  assert(status == EZO_STATUS_SUCCESS);
  assert(response_len == 3);
  assert(buffer[0] == 'O');
  assert(buffer[1] == 'K');
  assert(buffer[2] == 0);
}

static void test_cpp_wrapper_tracks_address_changes(void) {
  ezo_fake_transport_t fake;
  ezo_i2c::Device device;

  ezo_fake_transport_init(&fake);
  assert(device.init(97, ezo_fake_transport_vtable(), &fake) == EZO_OK);
  assert(device.address() == 97);

  device.set_address(98);
  assert(device.address() == 98);
}

static void test_cpp_wrapper_requires_explicit_init(void) {
  ezo_i2c::Device device;
  assert(device.send_read(NULL) == EZO_ERR_INVALID_ARGUMENT);
}

int main() {
  test_cpp_wrapper_send_read_and_parse();
  test_cpp_wrapper_reads_raw_response();
  test_cpp_wrapper_tracks_address_changes();
  test_cpp_wrapper_requires_explicit_init();
  return 0;
}
