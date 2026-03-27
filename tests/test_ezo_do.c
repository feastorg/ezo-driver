#include "ezo_do.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_helpers_cover_reading_output_config_and_queries(void) {
  ezo_do_reading_t reading;
  ezo_do_output_config_t output_config;
  ezo_do_temperature_compensation_t temperature;
  ezo_do_salinity_compensation_t salinity;
  ezo_do_pressure_compensation_t pressure;
  ezo_do_calibration_status_t calibration;

  assert(ezo_do_parse_reading("8.42",
                              strlen("8.42"),
                              EZO_DO_OUTPUT_MG_L | 0x80u,
                              &reading) == EZO_OK);
  assert(reading.present_mask == EZO_DO_OUTPUT_MG_L);
  assert(reading.milligrams_per_liter > 8.41 && reading.milligrams_per_liter < 8.43);
  assert(reading.percent_saturation == 0.0);

  assert(ezo_do_parse_output_config("?,O,mg,%", strlen("?,O,mg,%"), &output_config) == EZO_OK);
  assert(output_config.enabled_mask ==
         (EZO_DO_OUTPUT_MG_L | EZO_DO_OUTPUT_PERCENT_SATURATION));
  assert(ezo_do_parse_output_config("?O,mg,%", strlen("?O,mg,%"), &output_config) == EZO_OK);
  assert(ezo_do_parse_output_config("?O,MG", strlen("?O,MG"), &output_config) == EZO_OK);
  assert(output_config.enabled_mask == EZO_DO_OUTPUT_MG_L);
  assert(ezo_do_parse_output_config("?O,MG,1,%,0", strlen("?O,MG,1,%,0"), &output_config) ==
         EZO_OK);
  assert(output_config.enabled_mask == EZO_DO_OUTPUT_MG_L);
  assert(ezo_do_parse_output_config("?O,MG,0,%,1", strlen("?O,MG,0,%,1"), &output_config) ==
         EZO_OK);
  assert(output_config.enabled_mask == EZO_DO_OUTPUT_PERCENT_SATURATION);

  assert(ezo_do_parse_temperature("?T,22.4", strlen("?T,22.4"), &temperature) == EZO_OK);
  assert(temperature.temperature_c > 22.3 && temperature.temperature_c < 22.5);

  assert(ezo_do_parse_salinity("?S,12.5,ppt", strlen("?S,12.5,ppt"), &salinity) == EZO_OK);
  assert(salinity.value > 12.4 && salinity.value < 12.6);
  assert(salinity.unit == EZO_DO_SALINITY_UNIT_PPT);

  assert(ezo_do_parse_pressure("?,P,101.3", strlen("?,P,101.3"), &pressure) == EZO_OK);
  assert(pressure.pressure_kpa > 101.2 && pressure.pressure_kpa < 101.4);
  assert(ezo_do_parse_pressure("?P,101.3", strlen("?P,101.3"), &pressure) == EZO_OK);

  assert(ezo_do_parse_calibration_status("?Cal,2", strlen("?Cal,2"), &calibration) == EZO_OK);
  assert(calibration.level == 2U);
}

static void test_command_builders_format_expected_commands(void) {
  char command[32];

  assert(ezo_do_build_output_command(command, sizeof(command), EZO_DO_OUTPUT_PERCENT_SATURATION,
                                     0) == EZO_OK);
  assert(strcmp(command, "O,%,0") == 0);

  assert(ezo_do_build_temperature_command(command, sizeof(command), 25.0, 1) == EZO_OK);
  assert(strcmp(command, "T,25.0") == 0);

  assert(ezo_do_build_salinity_command(command, sizeof(command), 1200.0,
                                       EZO_DO_SALINITY_UNIT_MICROSIEMENS, 1) == EZO_OK);
  assert(strcmp(command, "S,1200.0,uS") == 0);

  assert(ezo_do_build_pressure_command(command, sizeof(command), 101.3, 2) == EZO_OK);
  assert(strcmp(command, "P,101.30") == 0);

  assert(ezo_do_build_calibration_command(command,
                                          sizeof(command),
                                          EZO_DO_CALIBRATION_ZERO) == EZO_OK);
  assert(strcmp(command, "Cal,0") == 0);
}

static void test_i2c_helpers_send_and_parse_typed_responses(void) {
  static const uint8_t output_response[] = {1, '?', 'O', ',', 'M', 'G', 0};
  static const uint8_t salinity_response[] = {1, '?', 'S', ',', '1', '4', '.', '7', ',',
                                              'p', 'p', 't', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_do_output_config_t output_config;
  ezo_do_salinity_compensation_t salinity;
  ezo_do_reading_t reading;
  ezo_do_calibration_status_t calibration;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 97, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_do_send_read_with_temp_comp_i2c(&device, 21.5, 2, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert(fake.last_tx_len == strlen("rt,21.50"));
  assert(memcmp(fake.last_tx_bytes, "rt,21.50", strlen("rt,21.50")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '8', '.', '5', '0', ',', '9', '4',
                                                           '.', '2', 0},
                                  11);
  assert(ezo_do_read_response_i2c(&device,
                                  EZO_DO_OUTPUT_MG_L |
                                      EZO_DO_OUTPUT_PERCENT_SATURATION,
                                  &reading) == EZO_OK);
  assert(reading.milligrams_per_liter > 8.49 && reading.milligrams_per_liter < 8.51);
  assert(reading.percent_saturation > 94.1 && reading.percent_saturation < 94.3);

  ezo_fake_i2c_transport_set_response(&fake, output_response, sizeof(output_response));
  assert(ezo_do_send_output_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_do_read_output_config_i2c(&device, &output_config) == EZO_OK);
  assert(output_config.enabled_mask == EZO_DO_OUTPUT_MG_L);

  ezo_fake_i2c_transport_set_response(&fake, salinity_response, sizeof(salinity_response));
  assert(ezo_do_send_salinity_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_do_read_salinity_i2c(&device, &salinity) == EZO_OK);
  assert(salinity.value > 14.6 && salinity.value < 14.8);
  assert(salinity.unit == EZO_DO_SALINITY_UNIT_PPT);

  assert(ezo_do_send_calibration_i2c(&device, EZO_DO_CALIBRATION_ATMOSPHERIC, &hint) == EZO_OK);
  assert(hint.wait_ms == 1300);
  assert(memcmp(fake.last_tx_bytes, "Cal", strlen("Cal")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, (const uint8_t[]){1, '?', 'C', 'a', 'l', ',', '1', 0},
                                  8);
  assert(ezo_do_send_calibration_query_i2c(&device, &hint) == EZO_OK);
  assert(ezo_do_read_calibration_status_i2c(&device, &calibration) == EZO_OK);
  assert(calibration.level == 1U);
}

static void test_uart_helpers_cover_plain_read_and_query_sequences(void) {
  static const uint8_t ok_response[] = {'*', 'O', 'K', '\r'};
  static const uint8_t read_then_pressure_response[] = {
      '9', '.', '1', '0', '\r', '*', 'O', 'K', '\r',
      '?', ',', 'P', ',', '9', '9', '.', '8', '\r', '*', 'O', 'K', '\r'};
  static const uint8_t output_response[] = {'?', ',', 'O', ',', 'm', 'g', ',', '%', '\r',
                                            '*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_do_reading_t reading;
  ezo_do_pressure_compensation_t pressure;
  ezo_do_output_config_t output_config;
  ezo_do_calibration_status_t calibration;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_do_send_salinity_set_uart(&device, 35.0, EZO_DO_SALINITY_UNIT_PPT, 1, &hint) ==
         EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.tx_len == strlen("S,35.0,ppt\r"));
  assert(memcmp(fake.tx_bytes, "S,35.0,ppt\r", strlen("S,35.0,ppt\r")) == 0);

  ezo_fake_uart_transport_set_response(&fake, ok_response, sizeof(ok_response));
  ezo_fake_uart_transport_append_response(&fake,
                                          read_then_pressure_response,
                                          sizeof(read_then_pressure_response));
  assert(ezo_uart_read_ok(&device) == EZO_OK);
  assert(ezo_do_send_read_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 600);
  assert(ezo_do_read_response_uart(&device, EZO_DO_OUTPUT_MG_L, &reading) == EZO_OK);
  assert(reading.milligrams_per_liter > 9.09 && reading.milligrams_per_liter < 9.11);

  assert(ezo_do_send_pressure_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_do_read_pressure_uart(&device, &pressure) == EZO_OK);
  assert(pressure.pressure_kpa > 99.7 && pressure.pressure_kpa < 99.9);

  ezo_fake_uart_transport_set_response(&fake, output_response, sizeof(output_response));
  assert(ezo_do_send_output_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_do_read_output_config_uart(&device, &output_config) == EZO_OK);
  assert(output_config.enabled_mask ==
         (EZO_DO_OUTPUT_MG_L | EZO_DO_OUTPUT_PERCENT_SATURATION));

  ezo_fake_uart_transport_set_response(&fake,
                                       (const uint8_t[]){'?', 'C', 'a', 'l', ',', '2', '\r', '*',
                                                         'O', 'K', '\r'},
                                       11);
  assert(ezo_do_send_calibration_query_uart(&device, &hint) == EZO_OK);
  assert(ezo_do_read_calibration_status_uart(&device, &calibration) == EZO_OK);
  assert(calibration.level == 2U);
}

int main(void) {
  test_parse_helpers_cover_reading_output_config_and_queries();
  test_command_builders_format_expected_commands();
  test_i2c_helpers_send_and_parse_typed_responses();
  test_uart_helpers_cover_plain_read_and_query_sequences();
  return 0;
}
