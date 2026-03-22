#include "ezo_hum.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_helpers_cover_reading_and_output_config(void) {
  ezo_hum_reading_t reading;
  ezo_hum_output_config_t output_config;
  ezo_hum_temperature_calibration_status_t calibration;

  assert(ezo_hum_parse_reading("48.5,9.2",
                               strlen("48.5,9.2"),
                               EZO_HUM_OUTPUT_HUMIDITY | EZO_HUM_OUTPUT_DEW_POINT | 0x80u,
                               &reading) == EZO_OK);
  assert(reading.present_mask ==
         (EZO_HUM_OUTPUT_HUMIDITY | EZO_HUM_OUTPUT_DEW_POINT));
  assert(reading.relative_humidity_percent > 48.4 && reading.relative_humidity_percent < 48.6);
  assert(reading.air_temperature_c == 0.0);
  assert(reading.dew_point_c > 9.1 && reading.dew_point_c < 9.3);

  assert(ezo_hum_parse_output_config("?O,HUM,T,Dew", strlen("?O,HUM,T,Dew"), &output_config) ==
         EZO_OK);
  assert(output_config.enabled_mask ==
         (EZO_HUM_OUTPUT_HUMIDITY | EZO_HUM_OUTPUT_AIR_TEMPERATURE |
          EZO_HUM_OUTPUT_DEW_POINT));

  assert(ezo_hum_parse_temperature_calibration_status("?Tcal,1",
                                                      strlen("?Tcal,1"),
                                                      &calibration) == EZO_OK);
  assert(calibration.calibrated == 1);
}

static void test_command_builders_format_expected_commands(void) {
  char command[16];

  assert(ezo_hum_build_output_command(command, sizeof(command), EZO_HUM_OUTPUT_AIR_TEMPERATURE,
                                      0) == EZO_OK);
  assert(strcmp(command, "O,T,0") == 0);

  assert(ezo_hum_build_output_command(command, sizeof(command), EZO_HUM_OUTPUT_DEW_POINT, 1) ==
         EZO_OK);
  assert(strcmp(command, "O,Dew,1") == 0);

  assert(ezo_hum_build_temperature_calibration_command(command, sizeof(command), 25.7, 1) ==
         EZO_OK);
  assert(strcmp(command, "Tcal,25.7") == 0);
}

static void test_i2c_helpers_send_and_parse_typed_responses(void) {
  static const uint8_t output_response[] = {1, '?', 'O', ',', 'H', 'U', 'M', ',', 'D',
                                            'e', 'w', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_hum_output_config_t output_config;
  ezo_hum_reading_t reading;
  ezo_hum_temperature_calibration_status_t calibration;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 111, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_hum_send_read_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.last_tx_len == 1);
  assert(fake.last_tx_bytes[0] == 'r');

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '4', '4', '.', '0', ',', '2', '1',
                                                           '.', '8', ',', '8', '.', '1', 0},
                                  15);
  assert(ezo_hum_read_response_i2c(&device,
                                   EZO_HUM_OUTPUT_HUMIDITY |
                                       EZO_HUM_OUTPUT_AIR_TEMPERATURE |
                                       EZO_HUM_OUTPUT_DEW_POINT,
                                   &reading) == EZO_OK);
  assert(reading.relative_humidity_percent > 43.9 && reading.relative_humidity_percent < 44.1);
  assert(reading.air_temperature_c > 21.7 && reading.air_temperature_c < 21.9);
  assert(reading.dew_point_c > 8.0 && reading.dew_point_c < 8.2);

  ezo_fake_i2c_transport_set_response(&fake, output_response, sizeof(output_response));
  assert(ezo_hum_send_output_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_hum_read_output_config_i2c(&device, &output_config) == EZO_OK);
  assert(output_config.enabled_mask ==
         (EZO_HUM_OUTPUT_HUMIDITY | EZO_HUM_OUTPUT_DEW_POINT));

  assert(ezo_hum_send_temperature_calibration_i2c(&device, 25.7, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(memcmp(fake.last_tx_bytes, "Tcal,25.7", strlen("Tcal,25.7")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '?', 'T', 'c', 'a', 'l', ',', '0',
                                                           0},
                                  9);
  assert(ezo_hum_send_temperature_calibration_query_i2c(&device, &hint) == EZO_OK);
  assert(ezo_hum_read_temperature_calibration_status_i2c(&device, &calibration) == EZO_OK);
  assert(calibration.calibrated == 0);
}

static void test_uart_helpers_cover_plain_read_and_query_sequences(void) {
  static const uint8_t read_then_output_response[] = {
      '5', '0', '.', '1', ',', '2', '2', '.', '4', '\r', '*', 'O', 'K', '\r',
      '?', 'O', ',', 'H', 'U', 'M', ',', 'T', '\r', '*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_hum_reading_t reading;
  ezo_hum_output_config_t output_config;
  ezo_hum_temperature_calibration_status_t calibration;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_hum_send_output_set_uart(&device, EZO_HUM_OUTPUT_HUMIDITY, 0, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.tx_len == strlen("O,HUM,0\r"));
  assert(memcmp(fake.tx_bytes, "O,HUM,0\r", strlen("O,HUM,0\r")) == 0);

  ezo_fake_uart_transport_set_response(&fake,
                                       read_then_output_response,
                                       sizeof(read_then_output_response));
  assert(ezo_hum_send_read_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 1000);
  assert(ezo_hum_read_response_uart(&device,
                                    EZO_HUM_OUTPUT_HUMIDITY |
                                        EZO_HUM_OUTPUT_AIR_TEMPERATURE,
                                    &reading) == EZO_OK);
  assert(reading.relative_humidity_percent > 50.0 && reading.relative_humidity_percent < 50.2);
  assert(reading.air_temperature_c > 22.3 && reading.air_temperature_c < 22.5);

  assert(ezo_hum_send_output_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_hum_read_output_config_uart(&device, &output_config) == EZO_OK);
  assert(output_config.enabled_mask ==
         (EZO_HUM_OUTPUT_HUMIDITY | EZO_HUM_OUTPUT_AIR_TEMPERATURE));

  ezo_fake_uart_transport_set_response(&fake,
                                       (const uint8_t[]){'?', 'T', 'c', 'a', 'l', ',', '1', '\r',
                                                         '*', 'O', 'K', '\r'},
                                       12);
  assert(ezo_hum_send_temperature_calibration_query_uart(&device, &hint) == EZO_OK);
  assert(ezo_hum_read_temperature_calibration_status_uart(&device, &calibration) == EZO_OK);
  assert(calibration.calibrated == 1);
}

int main(void) {
  test_parse_helpers_cover_reading_and_output_config();
  test_command_builders_format_expected_commands();
  test_i2c_helpers_send_and_parse_typed_responses();
  test_uart_helpers_cover_plain_read_and_query_sequences();
  return 0;
}
