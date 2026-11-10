#include "ezo_o2.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_reading_follows_the_enabled_outputs(void) {
  ezo_o2_reading_t reading;

  assert(ezo_o2_parse_reading("20.95", strlen("20.95"), EZO_O2_OUTPUT_PERCENT, &reading) == EZO_OK);
  assert(reading.present_mask == EZO_O2_OUTPUT_PERCENT);
  assert(reading.percent > 20.94 && reading.percent < 20.96);
  assert(reading.parts_per_thousand == 0.0);

  assert(ezo_o2_parse_reading("209.5", strlen("209.5"), EZO_O2_OUTPUT_PPT, &reading) == EZO_OK);
  assert(reading.present_mask == EZO_O2_OUTPUT_PPT);
  assert(reading.percent == 0.0);
  assert(reading.parts_per_thousand > 209.4 && reading.parts_per_thousand < 209.6);

  /* Percent first is inferred from `?,O,%,PPT`; no datasheet page prints a two-value reading. */
  assert(ezo_o2_parse_reading("20.95,209.5",
                              strlen("20.95,209.5"),
                              EZO_O2_OUTPUT_PERCENT | EZO_O2_OUTPUT_PPT | 0x80u,
                              &reading) == EZO_OK);
  assert(reading.present_mask == (EZO_O2_OUTPUT_PERCENT | EZO_O2_OUTPUT_PPT));
  assert(reading.percent > 20.94 && reading.percent < 20.96);
  assert(reading.parts_per_thousand > 209.4 && reading.parts_per_thousand < 209.6);
}

static void test_parse_reading_rejects_a_field_count_mismatch(void) {
  ezo_o2_reading_t reading;

  assert(ezo_o2_parse_reading("20.95",
                              strlen("20.95"),
                              EZO_O2_OUTPUT_PERCENT | EZO_O2_OUTPUT_PPT,
                              &reading) == EZO_ERR_PARSE);
  assert(ezo_o2_parse_reading("20.95,209.5", strlen("20.95,209.5"), EZO_O2_OUTPUT_PERCENT,
                              &reading) == EZO_ERR_PARSE);
  assert(ezo_o2_parse_reading("no output", strlen("no output"), EZO_O2_OUTPUT_PERCENT, &reading) ==
         EZO_ERR_PARSE);
}

static void test_parse_output_config_accepts_both_prefix_spellings(void) {
  ezo_o2_output_config_t config;

  assert(ezo_o2_parse_output_config("?,O,%,PPT", strlen("?,O,%,PPT"), &config) == EZO_OK);
  assert(config.enabled_mask == (EZO_O2_OUTPUT_PERCENT | EZO_O2_OUTPUT_PPT));

  assert(ezo_o2_parse_output_config("?O,PPT", strlen("?O,PPT"), &config) == EZO_OK);
  assert(config.enabled_mask == EZO_O2_OUTPUT_PPT);

  assert(ezo_o2_parse_output_config("?,O,%", strlen("?,O,%"), &config) == EZO_OK);
  assert(config.enabled_mask == EZO_O2_OUTPUT_PERCENT);

  assert(ezo_o2_parse_output_config("?,O,RGB", strlen("?,O,RGB"), &config) == EZO_ERR_PARSE);
  assert(ezo_o2_parse_output_config("?T,19.5", strlen("?T,19.5"), &config) == EZO_ERR_PARSE);
}

static void test_parse_temperature(void) {
  ezo_o2_temperature_compensation_t temperature;

  assert(ezo_o2_parse_temperature("?T,19.5", strlen("?T,19.5"), &temperature) == EZO_OK);
  assert(temperature.temperature_c > 19.4 && temperature.temperature_c < 19.6);
  assert(ezo_o2_parse_temperature("?S,19.5", strlen("?S,19.5"), &temperature) == EZO_ERR_PARSE);
}

static void test_command_builders(void) {
  char command[16];

  assert(ezo_o2_build_output_command(command, sizeof(command), EZO_O2_OUTPUT_PERCENT, 1) == EZO_OK);
  assert(strcmp(command, "O,%,1") == 0);
  assert(ezo_o2_build_output_command(command, sizeof(command), EZO_O2_OUTPUT_PERCENT, 0) == EZO_OK);
  assert(strcmp(command, "O,%,0") == 0);
  assert(ezo_o2_build_output_command(command, sizeof(command), EZO_O2_OUTPUT_PPT, 1) == EZO_OK);
  assert(strcmp(command, "O,PPT,1") == 0);
  assert(ezo_o2_build_output_command(command, sizeof(command), EZO_O2_OUTPUT_PPT, 0) == EZO_OK);
  assert(strcmp(command, "O,PPT,0") == 0);
  assert(ezo_o2_build_output_command(command, sizeof(command), 0x80u, 1) ==
         EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_o2_build_output_command(command, 3, EZO_O2_OUTPUT_PPT, 1) ==
         EZO_ERR_BUFFER_TOO_SMALL);

  assert(ezo_o2_build_temperature_command(command, sizeof(command), 19.5, 1) == EZO_OK);
  assert(strcmp(command, "T,19.5") == 0);
}

static void assert_i2c_sent(const ezo_fake_i2c_transport_t *fake, const char *expected) {
  assert(fake->last_tx_len == strlen(expected));
  assert(memcmp(fake->last_tx_bytes, expected, strlen(expected)) == 0);
}

static void test_i2c_helpers(void) {
  static const uint8_t read_response[] = {1, '2', '0', '.', '9', '5', 0};
  static const uint8_t output_response[] = {1, '?', ',', 'O', ',', '%', ',', 'P', 'P', 'T', 0};
  static const uint8_t temperature_response[] = {1, '?', 'T', ',', '1', '9', '.', '5', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_o2_reading_t reading;
  ezo_o2_output_config_t config;
  ezo_o2_temperature_compensation_t temperature;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 108, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_o2_send_read_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert_i2c_sent(&fake, "r");

  ezo_fake_i2c_transport_set_response(&fake, read_response, sizeof(read_response));
  assert(ezo_o2_read_response_i2c(&device, EZO_O2_OUTPUT_PERCENT, &reading) == EZO_OK);
  assert(reading.percent > 20.94 && reading.percent < 20.96);

  assert(ezo_o2_send_read_with_temp_comp_i2c(&device, 19.5, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert_i2c_sent(&fake, "rt,19.5");

  assert(ezo_o2_send_temperature_set_i2c(&device, 19.5, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert_i2c_sent(&fake, "T,19.5");

  assert(ezo_o2_send_temperature_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert_i2c_sent(&fake, "T,?");
  ezo_fake_i2c_transport_set_response(&fake, temperature_response, sizeof(temperature_response));
  assert(ezo_o2_read_temperature_i2c(&device, &temperature) == EZO_OK);
  assert(temperature.temperature_c > 19.4 && temperature.temperature_c < 19.6);

  assert(ezo_o2_send_output_set_i2c(&device, EZO_O2_OUTPUT_PPT, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert_i2c_sent(&fake, "O,PPT,1");

  assert(ezo_o2_send_output_query_i2c(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert_i2c_sent(&fake, "O,?");
  ezo_fake_i2c_transport_set_response(&fake, output_response, sizeof(output_response));
  assert(ezo_o2_read_output_config_i2c(&device, &config) == EZO_OK);
  assert(config.enabled_mask == (EZO_O2_OUTPUT_PERCENT | EZO_O2_OUTPUT_PPT));
}

static void assert_uart_sent(const ezo_fake_uart_transport_t *fake, const char *expected) {
  assert(fake->tx_len == strlen(expected));
  assert(memcmp(fake->tx_bytes, expected, strlen(expected)) == 0);
}

static void set_uart_reply(ezo_fake_uart_transport_t *fake, const char *reply) {
  ezo_fake_uart_transport_set_response(fake, (const uint8_t *)reply, strlen(reply));
}

static void test_uart_helpers(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_o2_reading_t reading;
  ezo_o2_output_config_t config;
  ezo_o2_temperature_compensation_t temperature;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_o2_send_read_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert_uart_sent(&fake, "r\r");
  set_uart_reply(&fake, "20.95\r*OK\r");
  assert(ezo_o2_read_response_uart(&device, EZO_O2_OUTPUT_PERCENT, &reading) == EZO_OK);
  assert(reading.percent > 20.94 && reading.percent < 20.96);

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_o2_send_temperature_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert_uart_sent(&fake, "T,?\r");
  set_uart_reply(&fake, "?T,19.5\r*OK\r");
  assert(ezo_o2_read_temperature_uart(&device, &temperature) == EZO_OK);
  assert(temperature.temperature_c > 19.4 && temperature.temperature_c < 19.6);

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_o2_send_temperature_set_uart(&device, 19.5, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert_uart_sent(&fake, "T,19.5\r");

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_o2_send_output_set_uart(&device, EZO_O2_OUTPUT_PERCENT, 0, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert_uart_sent(&fake, "O,%,0\r");

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_o2_send_output_query_uart(&device, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert_uart_sent(&fake, "O,?\r");
  set_uart_reply(&fake, "?,O,%,PPT\r*OK\r");
  assert(ezo_o2_read_output_config_uart(&device, &config) == EZO_OK);
  assert(config.enabled_mask == (EZO_O2_OUTPUT_PERCENT | EZO_O2_OUTPUT_PPT));
}

static void test_uart_read_with_temp_comp_expects_ok_before_the_value(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_o2_reading_t reading;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_o2_send_read_with_temp_comp_uart(&device, 19.5, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 900);
  assert_uart_sent(&fake, "rt,19.5\r");

  set_uart_reply(&fake, "*OK\r20.95\r");
  assert(ezo_o2_read_response_with_temp_comp_uart(&device, EZO_O2_OUTPUT_PERCENT, &reading) ==
         EZO_OK);
  assert(reading.percent > 20.94 && reading.percent < 20.96);

  set_uart_reply(&fake, "20.95\r*OK\r");
  assert(ezo_o2_read_response_with_temp_comp_uart(&device, EZO_O2_OUTPUT_PERCENT, &reading) ==
         EZO_ERR_PROTOCOL);

  set_uart_reply(&fake, "*OK\r*OK\r");
  assert(ezo_o2_read_response_uart(&device, EZO_O2_OUTPUT_PERCENT, &reading) == EZO_ERR_PROTOCOL);
}

int main(void) {
  test_parse_reading_follows_the_enabled_outputs();
  test_parse_reading_rejects_a_field_count_mismatch();
  test_parse_output_config_accepts_both_prefix_spellings();
  test_parse_temperature();
  test_command_builders();
  test_i2c_helpers();
  test_uart_helpers();
  test_uart_read_with_temp_comp_expects_ok_before_the_value();
  return 0;
}
