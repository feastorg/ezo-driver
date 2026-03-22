#include "ezo_rtd.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_helpers_cover_reading_and_queries(void) {
  ezo_rtd_reading_t reading;
  ezo_rtd_scale_status_t scale;
  ezo_rtd_calibration_status_t calibration;
  ezo_rtd_logger_status_t logger;
  ezo_rtd_memory_status_t memory;
  ezo_rtd_memory_entry_t entry;
  ezo_rtd_memory_value_t values[4];
  size_t value_count = 0;

  assert(ezo_rtd_parse_reading("25.104",
                               strlen("25.104"),
                               EZO_RTD_SCALE_CELSIUS,
                               &reading) == EZO_OK);
  assert(reading.temperature > 25.10 && reading.temperature < 25.11);
  assert(reading.scale == EZO_RTD_SCALE_CELSIUS);

  assert(ezo_rtd_parse_scale("?S,k", strlen("?S,k"), &scale) == EZO_OK);
  assert(scale.scale == EZO_RTD_SCALE_KELVIN);

  assert(ezo_rtd_parse_calibration_status("?Cal,1", strlen("?Cal,1"), &calibration) == EZO_OK);
  assert(calibration.calibrated == 1);

  assert(ezo_rtd_parse_logger_status("?D,6", strlen("?D,6"), &logger) == EZO_OK);
  assert(logger.interval_units == 6U);

  assert(ezo_rtd_parse_memory_status("?M,4", strlen("?M,4"), &memory) == EZO_OK);
  assert(memory.last_index == 4U);

  assert(ezo_rtd_parse_memory_entry("4,112.00",
                                    strlen("4,112.00"),
                                    EZO_RTD_SCALE_FAHRENHEIT,
                                    &entry) == EZO_OK);
  assert(entry.index == 4U);
  assert(entry.temperature > 111.9 && entry.temperature < 112.1);

  assert(ezo_rtd_parse_memory_all("100.00,104.00,108.00,112.00",
                                  strlen("100.00,104.00,108.00,112.00"),
                                  EZO_RTD_SCALE_FAHRENHEIT,
                                  values,
                                  4,
                                  &value_count) == EZO_OK);
  assert(value_count == 4U);
  assert(values[0].temperature > 99.9 && values[0].temperature < 100.1);
  assert(values[3].temperature > 111.9 && values[3].temperature < 112.1);
  assert(values[3].scale == EZO_RTD_SCALE_FAHRENHEIT);
}

static void test_command_builders_format_expected_commands(void) {
  char command[32];

  assert(ezo_rtd_build_scale_command(command, sizeof(command), EZO_RTD_SCALE_FAHRENHEIT) ==
         EZO_OK);
  assert(strcmp(command, "S,f") == 0);

  assert(ezo_rtd_build_calibration_command(command, sizeof(command), 100.0, 2) == EZO_OK);
  assert(strcmp(command, "Cal,100.00") == 0);

  assert(ezo_rtd_build_logger_command(command, sizeof(command), 6) == EZO_OK);
  assert(strcmp(command, "D,6") == 0);
}

static void test_i2c_helpers_send_and_parse_typed_responses(void) {
  static const uint8_t scale_response[] = {1, '?', 'S', ',', 'f', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_rtd_scale_status_t scale;
  ezo_rtd_reading_t reading;
  ezo_rtd_memory_entry_t entry;
  ezo_rtd_memory_value_t values[4];
  size_t value_count = 0;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 102, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_rtd_send_read_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 600);
  assert(fake.last_tx_len == 1);
  assert(fake.last_tx_bytes[0] == 'r');

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '7', '7', '.', '0', 0}, 6);
  assert(ezo_rtd_read_response_i2c(&device, EZO_RTD_SCALE_FAHRENHEIT, &reading) == EZO_OK);
  assert(reading.temperature > 76.9 && reading.temperature < 77.1);
  assert(reading.scale == EZO_RTD_SCALE_FAHRENHEIT);

  ezo_fake_i2c_transport_set_response(&fake, scale_response, sizeof(scale_response));
  assert(ezo_rtd_send_scale_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_rtd_read_scale_i2c(&device, &scale) == EZO_OK);
  assert(scale.scale == EZO_RTD_SCALE_FAHRENHEIT);

  assert(ezo_rtd_send_calibration_i2c(&device, 100.0, 2, &hint) == EZO_OK);
  assert(hint.wait_ms == 600);
  assert(memcmp(fake.last_tx_bytes, "Cal,100.00", strlen("Cal,100.00")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '4', ',', '1', '1', '2', '.', '0',
                                                           '0', 0},
                                  10);
  assert(ezo_rtd_send_memory_next_i2c(&device, &hint) == EZO_OK);
  assert(ezo_rtd_read_memory_entry_i2c(&device, EZO_RTD_SCALE_FAHRENHEIT, &entry) == EZO_OK);
  assert(entry.index == 4U);

  ezo_fake_i2c_transport_set_response(&fake,
                                  (const uint8_t[]){1, '1', '0', '0', '.', '0', '0', ',', '1',
                                                   '0', '4', '.', '0', '0', ',', '1', '0', '8',
                                                   '.', '0', '0', ',', '1', '1', '2', '.', '0',
                                                   '0', 0},
                                  29);
  assert(ezo_rtd_send_memory_all_i2c(&device, &hint) == EZO_OK);
  assert(ezo_rtd_read_memory_all_i2c(&device,
                                     EZO_RTD_SCALE_FAHRENHEIT,
                                     values,
                                     4,
                                     &value_count) == EZO_OK);
  assert(value_count == 4U);
  assert(values[2].temperature > 107.9 && values[2].temperature < 108.1);
}

static void test_uart_helpers_cover_plain_read_and_query_sequences(void) {
  static const uint8_t read_then_scale_response[] = {
      '2', '5', '.', '1', '0', '4', '\r', '*', 'O', 'K', '\r',
      '?', 'S', ',', 'c', '\r', '*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_rtd_reading_t reading;
  ezo_rtd_scale_status_t scale;
  ezo_rtd_logger_status_t logger;
  ezo_rtd_memory_value_t values[4];
  size_t value_count = 0;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_rtd_send_scale_set_uart(&device, EZO_RTD_SCALE_KELVIN, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.tx_len == strlen("S,k\r"));
  assert(memcmp(fake.tx_bytes, "S,k\r", strlen("S,k\r")) == 0);

  ezo_fake_uart_transport_set_response(&fake,
                                       read_then_scale_response,
                                       sizeof(read_then_scale_response));
  assert(ezo_rtd_send_read_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 1000);
  assert(ezo_rtd_read_response_uart(&device, EZO_RTD_SCALE_CELSIUS, &reading) == EZO_OK);
  assert(reading.temperature > 25.10 && reading.temperature < 25.11);
  assert(reading.scale == EZO_RTD_SCALE_CELSIUS);

  assert(ezo_rtd_send_scale_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_rtd_read_scale_uart(&device, &scale) == EZO_OK);
  assert(scale.scale == EZO_RTD_SCALE_CELSIUS);

  ezo_fake_uart_transport_set_response(&fake,
                                       (const uint8_t[]){'?', 'D', ',', '6', '\r', '*', 'O', 'K',
                                                         '\r'},
                                       9);
  assert(ezo_rtd_send_logger_query_uart(&device, &hint) == EZO_OK);
  assert(ezo_rtd_read_logger_uart(&device, &logger) == EZO_OK);
  assert(logger.interval_units == 6U);

  ezo_fake_uart_transport_set_response(&fake,
                                       (const uint8_t[]){'1', '0', '0', '.', '0', '0', ',',
                                                         '1', '0', '4', '.', '0', '0', ',', '1',
                                                         '0', '8', '.', '0', '0', ',', '1', '1',
                                                         '2', '.', '0', '0', '\r', '*', 'O', 'K',
                                                         '\r'},
                                       32);
  assert(ezo_rtd_send_memory_all_uart(&device, &hint) == EZO_OK);
  assert(ezo_rtd_read_memory_all_uart(&device,
                                      EZO_RTD_SCALE_CELSIUS,
                                      values,
                                      4,
                                      &value_count) == EZO_OK);
  assert(value_count == 4U);
  assert(values[1].temperature > 103.9 && values[1].temperature < 104.1);
  assert(values[1].scale == EZO_RTD_SCALE_CELSIUS);
}

int main(void) {
  test_parse_helpers_cover_reading_and_queries();
  test_command_builders_format_expected_commands();
  test_i2c_helpers_send_and_parse_typed_responses();
  test_uart_helpers_cover_plain_read_and_query_sequences();
  return 0;
}
