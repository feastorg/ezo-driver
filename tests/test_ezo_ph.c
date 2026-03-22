#include "ezo_ph.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_helpers_cover_reading_and_queries(void) {
  ezo_ph_reading_t reading;
  ezo_ph_temperature_compensation_t temperature;
  ezo_ph_calibration_status_t calibration;
  ezo_ph_slope_t slope;
  ezo_ph_extended_range_status_t extended_range;

  assert(ezo_ph_parse_reading("7.156", strlen("7.156"), &reading) == EZO_OK);
  assert(reading.ph > 7.15 && reading.ph < 7.16);

  assert(ezo_ph_parse_temperature("?T,19.5", strlen("?T,19.5"), &temperature) == EZO_OK);
  assert(temperature.temperature_c > 19.4 && temperature.temperature_c < 19.6);

  assert(ezo_ph_parse_calibration_status("?Cal,2", strlen("?Cal,2"), &calibration) == EZO_OK);
  assert(calibration.level == EZO_PH_CALIBRATION_TWO_POINT);

  assert(ezo_ph_parse_slope("?Slope,99.7,100.3,-0.89",
                            strlen("?Slope,99.7,100.3,-0.89"),
                            &slope) == EZO_OK);
  assert(slope.acid_percent > 99.6 && slope.acid_percent < 99.8);
  assert(slope.base_percent > 100.2 && slope.base_percent < 100.4);
  assert(slope.neutral_mv > -0.90 && slope.neutral_mv < -0.88);

  assert(ezo_ph_parse_extended_range("?pHext,1", strlen("?pHext,1"), &extended_range) ==
         EZO_OK);
  assert(extended_range.enabled == EZO_PH_EXTENDED_RANGE_ENABLED);
}

static void test_command_builders_format_expected_commands(void) {
  char command[32];

  assert(ezo_ph_build_temperature_command(command, sizeof(command), 19.5, 2) == EZO_OK);
  assert(strcmp(command, "T,19.50") == 0);

  assert(ezo_ph_build_calibration_command(command,
                                          sizeof(command),
                                          EZO_PH_CALIBRATION_POINT_HIGH,
                                          10.0,
                                          2) == EZO_OK);
  assert(strcmp(command, "Cal,high,10.00") == 0);

  assert(ezo_ph_build_extended_range_command(command,
                                             sizeof(command),
                                             EZO_PH_EXTENDED_RANGE_DISABLED) == EZO_OK);
  assert(strcmp(command, "pHext,0") == 0);
}

static void test_i2c_helpers_send_and_parse_typed_responses(void) {
  static const uint8_t slope_response[] = {
      1, '?', 'S', 'l', 'o', 'p', 'e', ',', '9', '9', '.', '7', ',', '1',
      '0', '0', '.', '3', ',', '-', '0', '.', '8', '9', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_ph_slope_t slope;
  ezo_ph_reading_t reading;
  ezo_ph_extended_range_status_t extended_range;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 99, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_ph_send_read_with_temp_comp_i2c(&device, 19.5, 3, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert(fake.last_tx_len == strlen("rt,19.500"));
  assert(memcmp(fake.last_tx_bytes, "rt,19.500", strlen("rt,19.500")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '7', '.', '1', '5', 0}, 6);
  assert(ezo_ph_read_response_i2c(&device, &reading) == EZO_OK);
  assert(reading.ph > 7.14 && reading.ph < 7.16);

  ezo_fake_i2c_transport_set_response(&fake, slope_response, sizeof(slope_response));
  assert(ezo_ph_send_slope_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_ph_read_slope_i2c(&device, &slope) == EZO_OK);
  assert(slope.base_percent > 100.2 && slope.base_percent < 100.4);

  assert(ezo_ph_send_calibration_i2c(&device,
                                     EZO_PH_CALIBRATION_POINT_MID,
                                     7.0,
                                     2,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert(memcmp(fake.last_tx_bytes, "Cal,mid,7.00", strlen("Cal,mid,7.00")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '?', 'p', 'H', 'e', 'x', 't', ',',
                                                           '0', 0},
                                  10);
  assert(ezo_ph_send_extended_range_query_i2c(&device, &hint) == EZO_OK);
  assert(ezo_ph_read_extended_range_i2c(&device, &extended_range) == EZO_OK);
  assert(extended_range.enabled == EZO_PH_EXTENDED_RANGE_DISABLED);
}

static void test_uart_helpers_cover_plain_read_and_sequence_flows(void) {
  static const uint8_t read_then_temp_query_response[] = {
      '7', '.', '1', '5', '\r', '*', 'O', 'K', '\r',
      '?', 'T', ',', '1', '9', '.', '5', '\r', '*', 'O', 'K', '\r'};
  static const uint8_t temp_comp_read_response[] = {'*', 'O', 'K', '\r',
                                                    '8', '.', '9', '1', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_ph_reading_t reading;
  ezo_ph_temperature_compensation_t temperature;
  ezo_ph_extended_range_status_t extended_range;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_ph_send_temperature_set_uart(&device, 19.5, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.tx_len == strlen("T,19.5\r"));
  assert(memcmp(fake.tx_bytes, "T,19.5\r", strlen("T,19.5\r")) == 0);

  ezo_fake_uart_transport_set_response(&fake,
                                       read_then_temp_query_response,
                                       sizeof(read_then_temp_query_response));
  assert(ezo_ph_send_read_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert(ezo_ph_read_response_uart(&device, &reading) == EZO_OK);
  assert(reading.ph > 7.14 && reading.ph < 7.16);

  assert(ezo_ph_send_temperature_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_ph_read_temperature_uart(&device, &temperature) == EZO_OK);
  assert(temperature.temperature_c > 19.4 && temperature.temperature_c < 19.6);

  ezo_fake_uart_transport_set_response(&fake,
                                       temp_comp_read_response,
                                       sizeof(temp_comp_read_response));
  assert(ezo_ph_send_read_with_temp_comp_uart(&device, 19.5, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert(ezo_ph_read_response_with_temp_comp_uart(&device, &reading) == EZO_OK);
  assert(reading.ph > 8.90 && reading.ph < 8.92);

  assert(ezo_ph_send_clear_calibration_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);

  ezo_fake_uart_transport_set_response(&fake,
                                       (const uint8_t[]){'?', 'p', 'H', 'e', 'x', 't', ',', '1',
                                                         '\r', '*', 'O', 'K', '\r'},
                                       13);
  assert(ezo_ph_send_extended_range_query_uart(&device, &hint) == EZO_OK);
  assert(ezo_ph_read_extended_range_uart(&device, &extended_range) == EZO_OK);
  assert(extended_range.enabled == EZO_PH_EXTENDED_RANGE_ENABLED);
}

int main(void) {
  test_parse_helpers_cover_reading_and_queries();
  test_command_builders_format_expected_commands();
  test_i2c_helpers_send_and_parse_typed_responses();
  test_uart_helpers_cover_plain_read_and_sequence_flows();
  return 0;
}
