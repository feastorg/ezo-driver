#include "ezo_ec.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_helpers_cover_reading_output_config_and_queries(void) {
  ezo_ec_reading_t reading;
  ezo_ec_output_config_t output_config;
  ezo_ec_temperature_compensation_t temperature;
  ezo_ec_probe_k_t probe_k;
  ezo_ec_tds_factor_t tds_factor;
  ezo_ec_calibration_status_t calibration;

  assert(ezo_ec_parse_reading("412.0,1.025",
                              strlen("412.0,1.025"),
                              EZO_EC_OUTPUT_CONDUCTIVITY |
                                  EZO_EC_OUTPUT_SPECIFIC_GRAVITY | 0x80u,
                              &reading) == EZO_OK);
  assert(reading.present_mask ==
         (EZO_EC_OUTPUT_CONDUCTIVITY | EZO_EC_OUTPUT_SPECIFIC_GRAVITY));
  assert(reading.conductivity_us_cm > 411.9 && reading.conductivity_us_cm < 412.1);
  assert(reading.total_dissolved_solids_ppm == 0.0);
  assert(reading.salinity_ppt == 0.0);
  assert(reading.specific_gravity > 1.024 && reading.specific_gravity < 1.026);

  assert(ezo_ec_parse_output_config("?O,EC,S,SG", strlen("?O,EC,S,SG"), &output_config) ==
         EZO_OK);
  assert(output_config.enabled_mask ==
         (EZO_EC_OUTPUT_CONDUCTIVITY | EZO_EC_OUTPUT_SALINITY |
          EZO_EC_OUTPUT_SPECIFIC_GRAVITY));

  assert(ezo_ec_parse_temperature("?T,19.5", strlen("?T,19.5"), &temperature) == EZO_OK);
  assert(temperature.temperature_c > 19.4 && temperature.temperature_c < 19.6);

  assert(ezo_ec_parse_probe_k("?K,0.1", strlen("?K,0.1"), &probe_k) == EZO_OK);
  assert(probe_k.k_value > 0.09 && probe_k.k_value < 0.11);

  assert(ezo_ec_parse_tds_factor("?TDS,0.65", strlen("?TDS,0.65"), &tds_factor) == EZO_OK);
  assert(tds_factor.factor > 0.64 && tds_factor.factor < 0.66);

  assert(ezo_ec_parse_calibration_status("?Cal,3", strlen("?Cal,3"), &calibration) == EZO_OK);
  assert(calibration.level == 3U);
}

static void test_command_builders_format_expected_commands(void) {
  char command[32];

  assert(ezo_ec_build_output_command(command, sizeof(command), EZO_EC_OUTPUT_SALINITY, 0) ==
         EZO_OK);
  assert(strcmp(command, "O,S,0") == 0);

  assert(ezo_ec_build_temperature_command(command, sizeof(command), 23.75, 2) == EZO_OK);
  assert(strcmp(command, "T,23.75") == 0);

  assert(ezo_ec_build_probe_k_command(command, sizeof(command), 1.0, 2) == EZO_OK);
  assert(strcmp(command, "K,1.00") == 0);

  assert(ezo_ec_build_tds_factor_command(command, sizeof(command), 0.66, 3) == EZO_OK);
  assert(strcmp(command, "TDS,0.660") == 0);

  assert(ezo_ec_build_calibration_command(command,
                                          sizeof(command),
                                          EZO_EC_CALIBRATION_DRY,
                                          0.0,
                                          0) == EZO_OK);
  assert(strcmp(command, "Cal,dry") == 0);
}

static void test_i2c_helpers_send_and_parse_typed_responses(void) {
  static const uint8_t output_response[] = {1, '?', 'O', ',', 'E', 'C', ',', 'T',
                                            'D', 'S', ',', 'S', 'G', 0};
  static const uint8_t tds_factor_response[] = {1, '?', 'T', 'D', 'S', ',', '0', '.', '7', '1',
                                                0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_ec_output_config_t output_config;
  ezo_ec_tds_factor_t tds_factor;
  ezo_ec_reading_t reading;
  ezo_ec_calibration_status_t calibration;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_ec_send_read_with_temp_comp_i2c(&device, 19.5, 3, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert(fake.last_tx_len == strlen("rt,19.500"));
  assert(memcmp(fake.last_tx_bytes, "rt,19.500", strlen("rt,19.500")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '8', '4', '2', ',', '1', '.', '0',
                                                           '2', '1', 0},
                                  11);
  assert(ezo_ec_read_response_i2c(&device,
                                  EZO_EC_OUTPUT_CONDUCTIVITY |
                                      EZO_EC_OUTPUT_SPECIFIC_GRAVITY,
                                  &reading) == EZO_OK);
  assert(reading.conductivity_us_cm > 841.9 && reading.conductivity_us_cm < 842.1);
  assert(reading.specific_gravity > 1.020 && reading.specific_gravity < 1.022);

  ezo_fake_i2c_transport_set_response(&fake, output_response, sizeof(output_response));
  assert(ezo_ec_send_output_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_ec_read_output_config_i2c(&device, &output_config) == EZO_OK);
  assert(output_config.enabled_mask ==
         (EZO_EC_OUTPUT_CONDUCTIVITY | EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS |
          EZO_EC_OUTPUT_SPECIFIC_GRAVITY));

  ezo_fake_i2c_transport_set_response(&fake, tds_factor_response, sizeof(tds_factor_response));
  assert(ezo_ec_send_tds_factor_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_ec_read_tds_factor_i2c(&device, &tds_factor) == EZO_OK);
  assert(tds_factor.factor > 0.70 && tds_factor.factor < 0.72);

  assert(ezo_ec_send_calibration_i2c(&device,
                                     EZO_EC_CALIBRATION_LOW_POINT,
                                     12880.0,
                                     0,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 1200);
  assert(memcmp(fake.last_tx_bytes, "Cal,low,12880", strlen("Cal,low,12880")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '?', 'C', 'a', 'l', ',', '2', 0},
                                  8);
  assert(ezo_ec_send_calibration_query_i2c(&device, &hint) == EZO_OK);
  assert(ezo_ec_read_calibration_status_i2c(&device, &calibration) == EZO_OK);
  assert(calibration.level == 2U);
}

static void test_uart_helpers_cover_plain_read_and_query_sequences(void) {
  static const uint8_t read_response[] = {'5', '0', '0', '.', '0', ',', '2', '5', '0', '\r'};
  static const uint8_t output_response[] = {'?', 'O', ',', 'E', 'C', ',', 'S', '\r',
                                            '*', 'O', 'K', '\r'};
  static const uint8_t temperature_response[] = {'?', 'T', ',', '2', '4', '.', '5', '\r',
                                                 '*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_ec_reading_t reading;
  ezo_ec_output_config_t output_config;
  ezo_ec_temperature_compensation_t temperature;
  ezo_ec_calibration_status_t calibration;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_ec_send_probe_k_set_uart(&device, 1.0, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.tx_len == strlen("K,1.0\r"));
  assert(memcmp(fake.tx_bytes, "K,1.0\r", strlen("K,1.0\r")) == 0);

  ezo_fake_uart_transport_set_response(&fake, read_response, sizeof(read_response));
  assert(ezo_ec_send_read_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 600);
  assert(ezo_ec_read_response_uart(&device,
                                   EZO_EC_OUTPUT_CONDUCTIVITY |
                                       EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS,
                                   &reading) == EZO_OK);
  assert(reading.conductivity_us_cm > 499.9 && reading.conductivity_us_cm < 500.1);
  assert(reading.total_dissolved_solids_ppm > 249.9 && reading.total_dissolved_solids_ppm <
                                                       250.1);

  ezo_fake_uart_transport_set_response(&fake, output_response, sizeof(output_response));
  assert(ezo_ec_send_output_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_ec_read_output_config_uart(&device, &output_config) == EZO_OK);
  assert(output_config.enabled_mask ==
         (EZO_EC_OUTPUT_CONDUCTIVITY | EZO_EC_OUTPUT_SALINITY));

  ezo_fake_uart_transport_set_response(&fake, temperature_response, sizeof(temperature_response));
  assert(ezo_ec_send_temperature_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_ec_read_temperature_uart(&device, &temperature) == EZO_OK);
  assert(temperature.temperature_c > 24.4 && temperature.temperature_c < 24.6);

  ezo_fake_uart_transport_set_response(&fake,
                                       (const uint8_t[]){'?', 'C', 'a', 'l', ',', '1', '\r', '*',
                                                         'O', 'K', '\r'},
                                       11);
  assert(ezo_ec_send_calibration_query_uart(&device, &hint) == EZO_OK);
  assert(ezo_ec_read_calibration_status_uart(&device, &calibration) == EZO_OK);
  assert(calibration.level == 1U);
}

int main(void) {
  test_parse_helpers_cover_reading_output_config_and_queries();
  test_command_builders_format_expected_commands();
  test_i2c_helpers_send_and_parse_typed_responses();
  test_uart_helpers_cover_plain_read_and_query_sequences();
  return 0;
}