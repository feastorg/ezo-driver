#include "ezo_control.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_helpers_cover_shared_control_queries(void) {
  ezo_control_name_t name;
  ezo_control_status_t status;
  ezo_control_led_status_t led;
  ezo_control_protocol_lock_status_t protocol_lock;
  ezo_control_baud_status_t baud;
  ezo_control_response_code_status_t response_code;

  assert(ezo_control_parse_name("?Name,TANK-A", strlen("?Name,TANK-A"), &name) == EZO_OK);
  assert(strcmp(name.name, "TANK-A") == 0);
  assert(ezo_control_parse_name("?NAME,", strlen("?NAME,"), &name) == EZO_OK);
  assert(strcmp(name.name, "") == 0);

  assert(ezo_control_parse_status("?Status,P,5.038", strlen("?Status,P,5.038"), &status) ==
         EZO_OK);
  assert(status.restart_code == 'P');
  assert(status.supply_voltage > 5.03 && status.supply_voltage < 5.04);

  assert(ezo_control_parse_status("?STATUS,P,4.89", strlen("?STATUS,P,4.89"), &status) ==
         EZO_OK);
  assert(status.restart_code == 'P');
  assert(status.supply_voltage > 4.88 && status.supply_voltage < 4.90);

  assert(ezo_control_parse_led("?L,1", strlen("?L,1"), &led) == EZO_OK);
  assert(led.enabled == 1);

  assert(ezo_control_parse_protocol_lock("?Plock,0", strlen("?Plock,0"), &protocol_lock) ==
         EZO_OK);
  assert(protocol_lock.enabled == 0);

  assert(ezo_control_parse_baud("?Baud,38400", strlen("?Baud,38400"), &baud) == EZO_OK);
  assert(baud.baud_rate == 38400U);

  assert(ezo_control_parse_response_code("?*OK,0", strlen("?*OK,0"), &response_code) ==
         EZO_OK);
  assert(response_code.enabled == 0);
}

static void test_builders_cover_shared_control_commands(void) {
  char command[48];

  assert(ezo_control_build_name_command(command, sizeof(command), "mix-tank") == EZO_OK);
  assert(strcmp(command, "Name,mix-tank") == 0);

  assert(ezo_control_build_led_command(command, sizeof(command), 0) == EZO_OK);
  assert(strcmp(command, "L,0") == 0);

  assert(ezo_control_build_protocol_lock_command(command, sizeof(command), 1) == EZO_OK);
  assert(strcmp(command, "Plock,1") == 0);

  assert(ezo_control_build_switch_to_i2c_command(command, sizeof(command), 100) == EZO_OK);
  assert(strcmp(command, "I2C,100") == 0);

  assert(ezo_control_build_switch_to_uart_command(command, sizeof(command), 38400) == EZO_OK);
  assert(strcmp(command, "Baud,38400") == 0);

  assert(ezo_control_build_response_code_command(command, sizeof(command), 0) == EZO_OK);
  assert(strcmp(command, "*OK,0") == 0);

  assert(ezo_control_build_name_command(command, sizeof(command), "two words") ==
         EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_control_build_name_command(command, sizeof(command), "12345678901234567") ==
         EZO_ERR_INVALID_ARGUMENT);
}

static void test_i2c_helpers_send_and_parse_shared_control_responses(void) {
  static const uint8_t info_response[] = {1, '?', 'i', ',', 'p', 'H', ',', '2', '.', '1', '6', 0};
  static const uint8_t observed_do_info_response[] = {1, '?', 'I', ',', 'D', 'O', ',', '2',
                                                      '.', '1', '6', 0};
  static const uint8_t status_response[] = {1, '?', 'S', 'T', 'A', 'T', 'U', 'S', ',', 'P', ',',
                                            '4', '.', '8', '9', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_device_info_t info;
  ezo_control_status_t status;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 99, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_control_send_name_set_i2c(&device, EZO_PRODUCT_PH, "tank", &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.last_tx_len == strlen("Name,tank"));
  assert(memcmp(fake.last_tx_bytes, "Name,tank", strlen("Name,tank")) == 0);

  ezo_fake_i2c_transport_set_response(&fake, info_response, sizeof(info_response));
  assert(ezo_control_send_info_query_i2c(&device, EZO_PRODUCT_PH, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_control_read_info_i2c(&device, &info) == EZO_OK);
  assert(info.product_id == EZO_PRODUCT_PH);
  assert(strcmp(info.firmware_version, "2.16") == 0);

  ezo_fake_i2c_transport_set_response(&fake,
                                      observed_do_info_response,
                                      sizeof(observed_do_info_response));
  assert(ezo_control_send_info_query_i2c(&device, EZO_PRODUCT_UNKNOWN, &hint) == EZO_OK);
  assert(ezo_control_read_info_i2c(&device, &info) == EZO_OK);
  assert(info.product_id == EZO_PRODUCT_DO);
  assert(strcmp(info.product_code, "DO") == 0);
  assert(strcmp(info.firmware_version, "2.16") == 0);

  ezo_fake_i2c_transport_set_response(&fake, status_response, sizeof(status_response));
  assert(ezo_control_send_status_query_i2c(&device, EZO_PRODUCT_PH, &hint) == EZO_OK);
  assert(ezo_control_read_status_i2c(&device, &status) == EZO_OK);
  assert(status.restart_code == 'P');
  assert(status.supply_voltage > 4.88 && status.supply_voltage < 4.90);

  assert(ezo_control_send_switch_to_uart_i2c(&device, EZO_PRODUCT_PH, 38400, &hint) == EZO_OK);
  assert(memcmp(fake.last_tx_bytes, "Baud,38400", strlen("Baud,38400")) == 0);
}

static void test_uart_helpers_send_and_parse_shared_control_sequences(void) {
  static const uint8_t ok_response[] = {'*', 'O', 'K', '\r'};
  static const uint8_t name_response[] = {'?', 'N', 'a', 'm', 'e', ',', 'm', 'i', 'x', '\r',
                                          '*', 'O', 'K', '\r'};
  static const uint8_t protocol_lock_response[] = {'?', 'P', 'l', 'o', 'c', 'k', ',', '1', '\r',
                                                   '*', 'O', 'K', '\r'};
  static const uint8_t baud_response[] = {'?', 'B', 'a', 'u', 'd', ',', '9', '6', '0', '0', '\r',
                                          '*', 'O', 'K', '\r'};
  static const uint8_t response_code_response[] = {'?', '*', 'O', 'K', ',', '0', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_control_name_t name;
  ezo_control_protocol_lock_status_t protocol_lock;
  ezo_control_baud_status_t baud;
  ezo_control_response_code_status_t response_code;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_control_send_led_set_uart(&device, EZO_PRODUCT_HUM, 1, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.tx_len == strlen("L,1\r"));
  assert(memcmp(fake.tx_bytes, "L,1\r", strlen("L,1\r")) == 0);

  ezo_fake_uart_transport_set_response(&fake, ok_response, sizeof(ok_response));
  ezo_fake_uart_transport_append_response(&fake, name_response, sizeof(name_response));
  assert(ezo_uart_read_ok(&device) == EZO_OK);
  assert(ezo_control_send_name_query_uart(&device, EZO_PRODUCT_HUM, &hint) == EZO_OK);
  assert(ezo_control_read_name_uart(&device, &name) == EZO_OK);
  assert(strcmp(name.name, "mix") == 0);

  ezo_fake_uart_transport_set_response(&fake,
                                       protocol_lock_response,
                                       sizeof(protocol_lock_response));
  assert(ezo_control_send_protocol_lock_query_uart(&device, EZO_PRODUCT_HUM, &hint) == EZO_OK);
  assert(ezo_control_read_protocol_lock_uart(&device, &protocol_lock) == EZO_OK);
  assert(protocol_lock.enabled == 1);

  ezo_fake_uart_transport_set_response(&fake, baud_response, sizeof(baud_response));
  assert(ezo_control_send_baud_query_uart(&device, EZO_PRODUCT_HUM, &hint) == EZO_OK);
  assert(ezo_control_read_baud_uart(&device, &baud) == EZO_OK);
  assert(baud.baud_rate == 9600U);

  assert(ezo_control_send_response_code_set_uart(&device, EZO_PRODUCT_HUM, 0, &hint) == EZO_OK);
  assert(memcmp(&fake.tx_bytes[fake.tx_len - strlen("*OK,0\r")], "*OK,0\r", strlen("*OK,0\r")) ==
         0);

  ezo_fake_uart_transport_set_response(&fake, ok_response, sizeof(ok_response));
  ezo_fake_uart_transport_append_response(&fake,
                                          response_code_response,
                                          sizeof(response_code_response));
  assert(ezo_uart_read_ok(&device) == EZO_OK);
  assert(ezo_control_send_response_code_query_uart(&device, EZO_PRODUCT_HUM, &hint) == EZO_OK);
  assert(ezo_control_read_response_code_uart(&device, &response_code) == EZO_OK);
  assert(response_code.enabled == 0);

  assert(ezo_control_send_switch_to_i2c_uart(&device, EZO_PRODUCT_HUM, 111, &hint) == EZO_OK);
  assert(memcmp(&fake.tx_bytes[fake.tx_len - strlen("I2C,111\r")],
                "I2C,111\r",
                strlen("I2C,111\r")) == 0);
}

static void test_uart_response_code_bootstrap_stays_explicit(void) {
  static const uint8_t response_code_disabled_response[] = {'?', '*', 'O', 'K', ',', '0', '\r'};
  static const uint8_t ok_response[] = {'*', 'O', 'K', '\r'};
  static const uint8_t name_response[] = {'?', 'N', 'a', 'm', 'e', ',', 'm', 'i', 'x', '\r',
                                          '*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_control_response_code_status_t response_code;
  ezo_control_name_t name;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  ezo_fake_uart_transport_set_response(&fake,
                                       response_code_disabled_response,
                                       sizeof(response_code_disabled_response));
  assert(ezo_control_send_response_code_query_uart(&device, EZO_PRODUCT_HUM, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_control_read_response_code_uart(&device, &response_code) == EZO_OK);
  assert(response_code.enabled == 0);

  ezo_fake_uart_transport_set_response(&fake, ok_response, sizeof(ok_response));
  ezo_fake_uart_transport_append_response(&fake, name_response, sizeof(name_response));
  assert(ezo_control_send_response_code_set_uart(&device, EZO_PRODUCT_HUM, 1, &hint) == EZO_OK);
  assert(ezo_uart_read_ok(&device) == EZO_OK);

  assert(ezo_control_send_name_query_uart(&device, EZO_PRODUCT_HUM, &hint) == EZO_OK);
  assert(ezo_control_read_name_uart(&device, &name) == EZO_OK);
  assert(strcmp(name.name, "mix") == 0);
}

int main(void) {
  test_parse_helpers_cover_shared_control_queries();
  test_builders_cover_shared_control_commands();
  test_i2c_helpers_send_and_parse_shared_control_responses();
  test_uart_helpers_send_and_parse_shared_control_sequences();
  test_uart_response_code_bootstrap_stays_explicit();
  return 0;
}
