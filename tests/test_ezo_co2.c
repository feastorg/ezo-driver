#include "ezo_co2.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_reading_follows_the_enabled_outputs(void) {
  ezo_co2_reading_t reading;

  assert(ezo_co2_parse_reading("6500", strlen("6500"), EZO_CO2_OUTPUT_PPM, &reading) == EZO_OK);
  assert(reading.present_mask == EZO_CO2_OUTPUT_PPM);
  assert(reading.ppm > 6499.9 && reading.ppm < 6500.1);
  assert(reading.internal_temperature_c == 0.0);
  assert(reading.warming == 0);

  /* ppm first is inferred from `?O,ppm,t`; no datasheet page prints a two-value reading. */
  assert(ezo_co2_parse_reading("800,25.1",
                               strlen("800,25.1"),
                               EZO_CO2_OUTPUT_PPM | EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE | 0x80u,
                               &reading) == EZO_OK);
  assert(reading.present_mask == (EZO_CO2_OUTPUT_PPM | EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE));
  assert(reading.ppm > 799.9 && reading.ppm < 800.1);
  assert(reading.internal_temperature_c > 25.0 && reading.internal_temperature_c < 25.2);
  assert(reading.warming == 0);
}

static void test_parse_reading_reports_warm_up(void) {
  ezo_co2_reading_t reading;

  memset(&reading, 0xFF, sizeof(reading));
  assert(ezo_co2_parse_reading("*warm", strlen("*warm"), EZO_CO2_OUTPUT_PPM, &reading) == EZO_OK);
  assert(reading.warming == 1);
  assert(reading.present_mask == 0U);
  assert(reading.ppm == 0.0);
  assert(reading.internal_temperature_c == 0.0);
}

static void test_parse_reading_rejects_other_text(void) {
  ezo_co2_reading_t reading;

  assert(ezo_co2_parse_reading("*war", strlen("*war"), EZO_CO2_OUTPUT_PPM, &reading) ==
         EZO_ERR_PARSE);
  assert(ezo_co2_parse_reading("*warmer", strlen("*warmer"), EZO_CO2_OUTPUT_PPM, &reading) ==
         EZO_ERR_PARSE);
  assert(ezo_co2_parse_reading("6500",
                               strlen("6500"),
                               EZO_CO2_OUTPUT_PPM | EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE,
                               &reading) == EZO_ERR_PARSE);
  assert(ezo_co2_parse_reading("6500,25.1", strlen("6500,25.1"), EZO_CO2_OUTPUT_PPM, &reading) ==
         EZO_ERR_PARSE);
}

static void test_parse_output_config(void) {
  ezo_co2_output_config_t config;

  assert(ezo_co2_parse_output_config("?O,ppm,t", strlen("?O,ppm,t"), &config) == EZO_OK);
  assert(config.enabled_mask == (EZO_CO2_OUTPUT_PPM | EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE));
  assert(ezo_co2_parse_output_config("?O,ppm", strlen("?O,ppm"), &config) == EZO_OK);
  assert(config.enabled_mask == EZO_CO2_OUTPUT_PPM);
  assert(ezo_co2_parse_output_config("?,O,ppm,t", strlen("?,O,ppm,t"), &config) == EZO_OK);
  assert(config.enabled_mask == (EZO_CO2_OUTPUT_PPM | EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE));
  assert(ezo_co2_parse_output_config("?O,RGB", strlen("?O,RGB"), &config) == EZO_ERR_PARSE);
  assert(ezo_co2_parse_output_config("?T,1", strlen("?T,1"), &config) == EZO_ERR_PARSE);
}

static void test_output_command_only_toggles_temperature(void) {
  char command[16];

  assert(ezo_co2_build_output_command(command, sizeof(command),
                                      EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE, 1) == EZO_OK);
  assert(strcmp(command, "O,t,1") == 0);
  assert(ezo_co2_build_output_command(command, sizeof(command),
                                      EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE, 0) == EZO_OK);
  assert(strcmp(command, "O,t,0") == 0);
  assert(ezo_co2_build_output_command(command, sizeof(command), EZO_CO2_OUTPUT_PPM, 1) ==
         EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_co2_build_output_command(command, 3, EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE, 1) ==
         EZO_ERR_BUFFER_TOO_SMALL);
}

static void test_i2c_helpers(void) {
  static const uint8_t read_response[] = {1, '6', '5', '0', '0', 0};
  static const uint8_t warm_response[] = {1, '*', 'w', 'a', 'r', 'm', 0};
  static const uint8_t output_response[] = {1, '?', 'O', ',', 'p', 'p', 'm', ',', 't', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_co2_reading_t reading;
  ezo_co2_output_config_t config;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 105, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_co2_send_read_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert(fake.last_tx_len == 1 && fake.last_tx_bytes[0] == 'r');

  ezo_fake_i2c_transport_set_response(&fake, read_response, sizeof(read_response));
  assert(ezo_co2_read_response_i2c(&device, EZO_CO2_OUTPUT_PPM, &reading) == EZO_OK);
  assert(reading.ppm > 6499.9 && reading.ppm < 6500.1);

  ezo_fake_i2c_transport_set_response(&fake, warm_response, sizeof(warm_response));
  assert(ezo_co2_read_response_i2c(&device, EZO_CO2_OUTPUT_PPM, &reading) == EZO_OK);
  assert(reading.warming == 1);

  assert(ezo_co2_send_output_set_i2c(&device, EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE, 1, &hint) ==
         EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.last_tx_len == strlen("O,t,1"));
  assert(memcmp(fake.last_tx_bytes, "O,t,1", strlen("O,t,1")) == 0);

  assert(ezo_co2_send_output_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.last_tx_len == strlen("O,?"));
  assert(memcmp(fake.last_tx_bytes, "O,?", strlen("O,?")) == 0);
  ezo_fake_i2c_transport_set_response(&fake, output_response, sizeof(output_response));
  assert(ezo_co2_read_output_config_i2c(&device, &config) == EZO_OK);
  assert(config.enabled_mask == (EZO_CO2_OUTPUT_PPM | EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE));
}

static void set_uart_reply(ezo_fake_uart_transport_t *fake, const char *reply) {
  ezo_fake_uart_transport_set_response(fake, (const uint8_t *)reply, strlen(reply));
}

static void test_uart_helpers(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_co2_reading_t reading;
  ezo_co2_output_config_t config;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_co2_send_read_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert(fake.tx_len == 2 && memcmp(fake.tx_bytes, "r\r", 2) == 0);
  set_uart_reply(&fake, "6500\r*OK\r");
  assert(ezo_co2_read_response_uart(&device, EZO_CO2_OUTPUT_PPM, &reading) == EZO_OK);
  assert(reading.ppm > 6499.9 && reading.ppm < 6500.1);

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_co2_send_output_set_uart(&device, EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE, 0, &hint) ==
         EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.tx_len == strlen("O,t,0\r"));
  assert(memcmp(fake.tx_bytes, "O,t,0\r", strlen("O,t,0\r")) == 0);

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_co2_send_output_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.tx_len == strlen("O,?\r"));
  assert(memcmp(fake.tx_bytes, "O,?\r", strlen("O,?\r")) == 0);
  set_uart_reply(&fake, "?O,ppm,t\r*OK\r");
  assert(ezo_co2_read_output_config_uart(&device, &config) == EZO_OK);
  assert(config.enabled_mask == (EZO_CO2_OUTPUT_PPM | EZO_CO2_OUTPUT_INTERNAL_TEMPERATURE));

  set_uart_reply(&fake, "*OK\r*OK\r");
  assert(ezo_co2_read_response_uart(&device, EZO_CO2_OUTPUT_PPM, &reading) == EZO_ERR_PROTOCOL);
}

int main(void) {
  test_parse_reading_follows_the_enabled_outputs();
  test_parse_reading_reports_warm_up();
  test_parse_reading_rejects_other_text();
  test_parse_output_config();
  test_output_command_only_toggles_temperature();
  test_i2c_helpers();
  test_uart_helpers();
  return 0;
}
