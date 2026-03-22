#include "ezo_orp.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_helpers_cover_reading_and_queries(void) {
  ezo_orp_reading_t reading;
  ezo_orp_calibration_status_t calibration;
  ezo_orp_extended_scale_status_t extended;

  assert(ezo_orp_parse_reading("225.4", strlen("225.4"), &reading) == EZO_OK);
  assert(reading.millivolts > 225.3 && reading.millivolts < 225.5);

  assert(ezo_orp_parse_calibration_status("?Cal,1", strlen("?Cal,1"), &calibration) == EZO_OK);
  assert(calibration.calibrated == 1);

  assert(ezo_orp_parse_extended_scale("?ORPext,0", strlen("?ORPext,0"), &extended) == EZO_OK);
  assert(extended.enabled == EZO_ORP_EXTENDED_SCALE_DISABLED);
}

static void test_command_builders_format_expected_commands(void) {
  char command[32];

  assert(ezo_orp_build_calibration_command(command, sizeof(command), 225.0, 0) == EZO_OK);
  assert(strcmp(command, "Cal,225") == 0);

  assert(ezo_orp_build_extended_scale_command(command,
                                              sizeof(command),
                                              EZO_ORP_EXTENDED_SCALE_ENABLED) == EZO_OK);
  assert(strcmp(command, "ORPext,1") == 0);
}

static void test_i2c_helpers_send_and_parse_typed_responses(void) {
  static const uint8_t calibration_response[] = {1, '?', 'C', 'a', 'l', ',', '1', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_orp_calibration_status_t calibration;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 98, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_orp_send_calibration_i2c(&device, 225.0, 0, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert(fake.last_tx_len == strlen("Cal,225"));
  assert(memcmp(fake.last_tx_bytes, "Cal,225", strlen("Cal,225")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, calibration_response, sizeof(calibration_response));
  assert(ezo_orp_send_calibration_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_orp_read_calibration_status_i2c(&device, &calibration) == EZO_OK);
  assert(calibration.calibrated == 1);
}

static void test_uart_helpers_cover_plain_read_and_query_sequences(void) {
  static const uint8_t read_response[] = {'2', '2', '5', '.', '4', '\r'};
  static const uint8_t extended_response[] = {'?', 'O', 'R', 'P', 'e', 'x', 't', ',', '1', '\r',
                                              '*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_orp_reading_t reading;
  ezo_orp_extended_scale_status_t extended;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  ezo_fake_uart_transport_set_response(&fake, read_response, sizeof(read_response));
  assert(ezo_orp_send_read_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 1000);
  assert(ezo_orp_read_response_uart(&device, &reading) == EZO_OK);
  assert(reading.millivolts > 225.3 && reading.millivolts < 225.5);

  ezo_fake_uart_transport_set_response(&fake, extended_response, sizeof(extended_response));
  assert(ezo_orp_send_extended_scale_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_orp_read_extended_scale_uart(&device, &extended) == EZO_OK);
  assert(extended.enabled == EZO_ORP_EXTENDED_SCALE_ENABLED);
}

int main(void) {
  test_parse_helpers_cover_reading_and_queries();
  test_command_builders_format_expected_commands();
  test_i2c_helpers_send_and_parse_typed_responses();
  test_uart_helpers_cover_plain_read_and_query_sequences();
  return 0;
}