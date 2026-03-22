#include "ezo.h"
#include "ezo_uart.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

static void assert_uart_line_kind(const uint8_t *response,
                                  size_t response_size,
                                  ezo_uart_response_kind_t expected_kind) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[16];
  size_t response_len = 0;

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, response_size);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) == EZO_OK);
  assert(kind == expected_kind);
}

static void test_uart_send_command_appends_carriage_return(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_send_command(&device, "i", EZO_COMMAND_GENERIC, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.write_call_count == 2);
  assert(fake.tx_len == 2);
  assert(memcmp(fake.tx_bytes, "i\r", 2) == 0);
}

static void test_uart_send_command_with_float_formats_value(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_send_command_with_float(&device,
                                          "rt,",
                                          25.0,
                                          3,
                                          EZO_COMMAND_READ_WITH_TEMP_COMP,
                                          &hint) == EZO_OK);
  assert(hint.wait_ms == 1000);
  assert(fake.tx_len == strlen("rt,25.000\r"));
  assert(memcmp(fake.tx_bytes, "rt,25.000\r", strlen("rt,25.000\r")) == 0);
}

static void test_uart_send_command_with_float_rounds_halfway_decimal_up(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_send_command_with_float(&device,
                                          "t,",
                                          2.675,
                                          2,
                                          EZO_COMMAND_GENERIC,
                                          NULL) == EZO_OK);
  assert(fake.tx_len == strlen("t,2.68\r"));
  assert(memcmp(fake.tx_bytes, "t,2.68\r", strlen("t,2.68\r")) == 0);
}

static void test_uart_send_command_with_float_rejects_non_finite_value(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_send_command_with_float(&device,
                                          "t,",
                                          INFINITY,
                                          2,
                                          EZO_COMMAND_GENERIC,
                                          NULL) == EZO_ERR_INVALID_ARGUMENT);
}

static void test_uart_send_command_rejects_embedded_terminator(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_send_command(&device, "r\n", EZO_COMMAND_READ, NULL) ==
         EZO_ERR_INVALID_ARGUMENT);
}

static void test_uart_read_line_classifies_data_and_parses(void) {
  static const uint8_t response[] = {'1', '2', '.', '3', '4', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[16];
  size_t response_len = 0;
  double value = 0.0;

  ezo_fake_uart_transport_init(&fake);
  fake.max_bytes_per_read = 2;
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) ==
         EZO_OK);
  assert(kind == EZO_UART_RESPONSE_DATA);
  assert(ezo_uart_device_get_last_response_kind(&device) == EZO_UART_RESPONSE_DATA);
  assert(response_len == 5);
  assert(memcmp(buffer, "12.34", response_len) == 0);
  assert(ezo_parse_double(buffer, response_len, &value) == EZO_OK);
  assert(value > 12.33 && value < 12.35);
}

static void test_uart_read_line_classifies_ok(void) {
  static const uint8_t response[] = {'*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) ==
         EZO_OK);
  assert(kind == EZO_UART_RESPONSE_OK);
  assert(response_len == 3);
  assert(memcmp(buffer, "*OK", 3) == 0);
}

static void test_uart_read_line_classifies_error(void) {
  static const uint8_t response[] = {'*', 'E', 'R', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) ==
         EZO_OK);
  assert(kind == EZO_UART_RESPONSE_ERROR);
  assert(response_len == 3);
  assert(memcmp(buffer, "*ER", 3) == 0);
}

static void test_uart_read_line_classifies_extended_control_tokens(void) {
  static const uint8_t over_voltage[] = {'*', 'O', 'V', '\r'};
  static const uint8_t under_voltage[] = {'*', 'U', 'V', '\r'};
  static const uint8_t reset[] = {'*', 'R', 'S', '\r'};
  static const uint8_t ready[] = {'*', 'R', 'E', '\r'};
  static const uint8_t sleep[] = {'*', 'S', 'L', '\r'};
  static const uint8_t wake[] = {'*', 'W', 'A', '\r'};
  static const uint8_t done[] = {'*', 'D', 'O', 'N', 'E', '\r'};

  assert_uart_line_kind(over_voltage, sizeof(over_voltage), EZO_UART_RESPONSE_OVER_VOLTAGE);
  assert_uart_line_kind(under_voltage, sizeof(under_voltage), EZO_UART_RESPONSE_UNDER_VOLTAGE);
  assert_uart_line_kind(reset, sizeof(reset), EZO_UART_RESPONSE_RESET);
  assert_uart_line_kind(ready, sizeof(ready), EZO_UART_RESPONSE_READY);
  assert_uart_line_kind(sleep, sizeof(sleep), EZO_UART_RESPONSE_SLEEP);
  assert_uart_line_kind(wake, sizeof(wake), EZO_UART_RESPONSE_WAKE);
  assert_uart_line_kind(done, sizeof(done), EZO_UART_RESPONSE_DONE);
  assert(ezo_uart_response_kind_is_control(EZO_UART_RESPONSE_DONE) == 1);
  assert(ezo_uart_response_kind_is_terminal(EZO_UART_RESPONSE_DONE) == 1);
  assert(ezo_uart_response_kind_is_terminal(EZO_UART_RESPONSE_WAKE) == 0);
}

static void test_uart_read_line_reads_data_then_terminal_status_as_sequence(void) {
  static const uint8_t response[] = {'1', '2', '.', '3', '4', '\r', '*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[16];
  size_t response_len = 0;

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) == EZO_OK);
  assert(kind == EZO_UART_RESPONSE_DATA);
  assert(response_len == 5);
  assert(memcmp(buffer, "12.34", 5) == 0);
  assert(ezo_uart_response_kind_is_control(kind) == 0);
  assert(ezo_uart_response_kind_is_terminal(kind) == 0);

  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) == EZO_OK);
  assert(kind == EZO_UART_RESPONSE_OK);
  assert(response_len == 3);
  assert(memcmp(buffer, "*OK", 3) == 0);
  assert(ezo_uart_response_kind_is_control(kind) == 1);
  assert(ezo_uart_response_kind_is_terminal(kind) == 1);
}

static void test_uart_read_line_surfaces_startup_control_tokens_explicitly(void) {
  static const uint8_t response[] = {'*', 'W', 'A', '\r', '*', 'R', 'E', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[16];
  size_t response_len = 0;

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) == EZO_OK);
  assert(kind == EZO_UART_RESPONSE_WAKE);
  assert(ezo_uart_response_kind_is_control(kind) == 1);
  assert(ezo_uart_response_kind_is_terminal(kind) == 0);

  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) == EZO_OK);
  assert(kind == EZO_UART_RESPONSE_READY);
  assert(ezo_uart_response_kind_is_control(kind) == 1);
  assert(ezo_uart_response_kind_is_terminal(kind) == 0);
}

static void test_uart_read_line_rejects_incomplete_line(void) {
  static const uint8_t response[] = {'1', '2', '.', '3', '4'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[16];
  size_t response_len = 0;

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) ==
         EZO_ERR_PROTOCOL);
  assert(kind == EZO_UART_RESPONSE_UNKNOWN);
  assert(response_len == 0);
}

static void test_uart_read_line_detects_buffer_too_small(void) {
  static const uint8_t response[] = {'1', '2', '3', '4', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[4];
  size_t response_len = 0;

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) ==
         EZO_ERR_BUFFER_TOO_SMALL);
  assert(kind == EZO_UART_RESPONSE_UNKNOWN);
  assert(response_len == 0);
}

static void test_uart_read_line_accepts_max_length_payload_with_capacity_buffer(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  uint8_t response[EZO_UART_MAX_TEXT_RESPONSE_CAPACITY];
  char buffer[EZO_UART_MAX_TEXT_RESPONSE_CAPACITY];
  size_t response_len = 0;
  size_t i = 0;

  for (i = 0; i < (size_t)EZO_UART_MAX_TEXT_RESPONSE_LEN; ++i) {
    response[i] = '7';
  }
  response[EZO_UART_MAX_TEXT_RESPONSE_LEN] = '\r';

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) == EZO_OK);
  assert(kind == EZO_UART_RESPONSE_DATA);
  assert(response_len == (size_t)EZO_UART_MAX_TEXT_RESPONSE_LEN);
  assert(buffer[response_len] == '\0');
}

static void test_uart_read_line_rejects_max_length_payload_without_capacity_buffer(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  uint8_t response[EZO_UART_MAX_TEXT_RESPONSE_CAPACITY];
  char buffer[EZO_UART_MAX_TEXT_RESPONSE_LEN];
  size_t response_len = 0;
  size_t i = 0;

  for (i = 0; i < (size_t)EZO_UART_MAX_TEXT_RESPONSE_LEN; ++i) {
    response[i] = '7';
  }
  response[EZO_UART_MAX_TEXT_RESPONSE_LEN] = '\r';

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) ==
         EZO_ERR_BUFFER_TOO_SMALL);
  assert(kind == EZO_UART_RESPONSE_UNKNOWN);
  assert(response_len == 0);
}

static void test_uart_send_command_propagates_transport_failure(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;

  ezo_fake_uart_transport_init(&fake);
  fake.write_result = EZO_ERR_TRANSPORT;
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_send_command(&device, "status", EZO_COMMAND_GENERIC, NULL) ==
         EZO_ERR_TRANSPORT);
}

static void test_uart_read_line_propagates_transport_failure(void) {
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_uart_transport_init(&fake);
  fake.read_result = EZO_ERR_TRANSPORT;
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) ==
         EZO_ERR_TRANSPORT);
  assert(kind == EZO_UART_RESPONSE_UNKNOWN);
  assert(response_len == 0);
}

static void test_uart_discard_input_uses_optional_transport_hook(void) {
  static const uint8_t response[] = {'1', '2', '\r', '*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_uart_transport_init(&fake);
  ezo_fake_uart_transport_set_response(&fake, response, sizeof(response));
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);
  assert(ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind) == EZO_OK);
  assert(kind == EZO_UART_RESPONSE_DATA);
  assert(ezo_uart_discard_input(&device) == EZO_OK);
  assert(fake.discard_call_count == 1);
  assert(fake.response_offset == fake.response_len);
  assert(ezo_uart_device_get_last_response_kind(&device) == EZO_UART_RESPONSE_UNKNOWN);
}

int main(void) {
  test_uart_send_command_appends_carriage_return();
  test_uart_send_command_with_float_formats_value();
  test_uart_send_command_with_float_rounds_halfway_decimal_up();
  test_uart_send_command_with_float_rejects_non_finite_value();
  test_uart_send_command_rejects_embedded_terminator();
  test_uart_read_line_classifies_data_and_parses();
  test_uart_read_line_classifies_ok();
  test_uart_read_line_classifies_error();
  test_uart_read_line_classifies_extended_control_tokens();
  test_uart_read_line_reads_data_then_terminal_status_as_sequence();
  test_uart_read_line_surfaces_startup_control_tokens_explicitly();
  test_uart_read_line_rejects_incomplete_line();
  test_uart_read_line_detects_buffer_too_small();
  test_uart_read_line_accepts_max_length_payload_with_capacity_buffer();
  test_uart_read_line_rejects_max_length_payload_without_capacity_buffer();
  test_uart_send_command_propagates_transport_failure();
  test_uart_read_line_propagates_transport_failure();
  test_uart_discard_input_uses_optional_transport_hook();
  return 0;
}
