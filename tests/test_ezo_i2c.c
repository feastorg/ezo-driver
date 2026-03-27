#include "ezo_i2c.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

static void test_send_command_records_bytes(void) {
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_result_t result;

  ezo_fake_i2c_transport_init(&fake);
  result = ezo_device_init(&device, 99, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_send_command(&device, "name,?", EZO_COMMAND_GENERIC, &hint);
  assert(result == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(fake.call_count == 1);
  assert(fake.last_tx_len == strlen("name,?"));
  assert(memcmp(fake.last_tx_bytes, "name,?", strlen("name,?")) == 0);
}

static void test_device_status_name_maps_public_statuses(void) {
  assert(strcmp(ezo_device_status_name(EZO_STATUS_UNKNOWN), "EZO_STATUS_UNKNOWN") == 0);
  assert(strcmp(ezo_device_status_name(EZO_STATUS_SUCCESS), "EZO_STATUS_SUCCESS") == 0);
  assert(strcmp(ezo_device_status_name(EZO_STATUS_FAIL), "EZO_STATUS_FAIL") == 0);
  assert(strcmp(ezo_device_status_name(EZO_STATUS_NOT_READY), "EZO_STATUS_NOT_READY") == 0);
  assert(strcmp(ezo_device_status_name(EZO_STATUS_NO_DATA), "EZO_STATUS_NO_DATA") == 0);
  assert(strcmp(ezo_device_status_name((ezo_device_status_t)99), "EZO_STATUS_INVALID") == 0);
}

static void test_send_command_with_float_formats_value(void) {
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_result_t result;

  ezo_fake_i2c_transport_init(&fake);
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_send_command_with_float(&device,
                                       "rt,",
                                       25.0,
                                       3,
                                       EZO_COMMAND_READ_WITH_TEMP_COMP,
                                       &hint);
  assert(result == EZO_OK);
  assert(hint.wait_ms == 1000);
  assert(fake.last_tx_len == strlen("rt,25.000"));
  assert(memcmp(fake.last_tx_bytes, "rt,25.000", strlen("rt,25.000")) == 0);
}

static void test_send_command_with_float_rounds_negative_values(void) {
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_result_t result;

  ezo_fake_i2c_transport_init(&fake);
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_send_command_with_float(&device,
                                       "t,",
                                       -1.236,
                                       2,
                                       EZO_COMMAND_GENERIC,
                                       NULL);
  assert(result == EZO_OK);
  assert(fake.last_tx_len == strlen("t,-1.24"));
  assert(memcmp(fake.last_tx_bytes, "t,-1.24", strlen("t,-1.24")) == 0);
}

static void test_send_command_with_float_rejects_excess_precision(void) {
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_result_t result;

  ezo_fake_i2c_transport_init(&fake);
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_send_command_with_float(&device,
                                       "t,",
                                       1.0,
                                       10,
                                       EZO_COMMAND_GENERIC,
                                       NULL);
  assert(result == EZO_ERR_INVALID_ARGUMENT);
}

static void test_send_command_with_float_rounds_halfway_decimal_up(void) {
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_result_t result;

  ezo_fake_i2c_transport_init(&fake);
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_send_command_with_float(&device,
                                       "t,",
                                       1.005,
                                       2,
                                       EZO_COMMAND_GENERIC,
                                       NULL);
  assert(result == EZO_OK);
  assert(fake.last_tx_len == strlen("t,1.01"));
  assert(memcmp(fake.last_tx_bytes, "t,1.01", strlen("t,1.01")) == 0);
}

static void test_send_command_with_float_rejects_non_finite_value(void) {
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_result_t result;

  ezo_fake_i2c_transport_init(&fake);
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_send_command_with_float(&device,
                                       "t,",
                                       NAN,
                                       2,
                                       EZO_COMMAND_GENERIC,
                                       NULL);
  assert(result == EZO_ERR_INVALID_ARGUMENT);
}

static void test_send_command_resets_last_device_status(void) {
  static const uint8_t response[] = {1, '7', '.', '1', '2', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  ezo_result_t result;
  char buffer[16];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  ezo_fake_i2c_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_OK);
  assert(ezo_device_get_last_status(&device) == EZO_STATUS_SUCCESS);

  result = ezo_send_command(&device, "status", EZO_COMMAND_GENERIC, NULL);
  assert(result == EZO_OK);
  assert(ezo_device_get_last_status(&device) == EZO_STATUS_UNKNOWN);
}

static void test_read_response_success_and_parse(void) {
  static const uint8_t response[] = {1, '7', '.', '1', '2', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status;
  ezo_result_t result;
  char buffer[16];
  size_t response_len = 0;
  double value = 0.0;

  ezo_fake_i2c_transport_init(&fake);
  ezo_fake_i2c_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_OK);
  assert(status == EZO_STATUS_SUCCESS);
  assert(response_len == 4);
  assert(memcmp(buffer, "7.12", 4) == 0);

  result = ezo_parse_double(buffer, response_len, &value);
  assert(result == EZO_OK);
  assert(value > 7.11 && value < 7.13);
}

static void test_read_response_raw_preserves_embedded_zero_bytes(void) {
  static const uint8_t response[] = {1, 'A', 0, 'B', 0x7f};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  ezo_result_t result;
  uint8_t buffer[8];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  ezo_fake_i2c_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response_raw(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_OK);
  assert(status == EZO_STATUS_SUCCESS);
  assert(response_len == 4);
  assert(buffer[0] == 'A');
  assert(buffer[1] == 0);
  assert(buffer[2] == 'B');
  assert(buffer[3] == 0x7f);
}

static void test_read_response_raw_detects_buffer_too_small(void) {
  static const uint8_t response[] = {1, '1', '2', '3', '4'};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  ezo_result_t result;
  uint8_t buffer[3];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  ezo_fake_i2c_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response_raw(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_ERR_BUFFER_TOO_SMALL);
  assert(status == EZO_STATUS_SUCCESS);
  assert(response_len == 0);
}

static void test_read_response_raw_not_ready_is_status_not_error(void) {
  static const uint8_t response[] = {254};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  ezo_result_t result;
  uint8_t buffer[4];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  ezo_fake_i2c_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response_raw(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_OK);
  assert(status == EZO_STATUS_NOT_READY);
  assert(response_len == 0);
}

static void test_read_response_not_ready_is_status_not_error(void) {
  static const uint8_t response[] = {254, 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status;
  ezo_result_t result;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  ezo_fake_i2c_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_OK);
  assert(status == EZO_STATUS_NOT_READY);
  assert(response_len == 0);
  assert(buffer[0] == '\0');
}

static void test_read_response_no_data_is_status_not_error(void) {
  static const uint8_t response[] = {255, 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status;
  ezo_result_t result;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  ezo_fake_i2c_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_OK);
  assert(status == EZO_STATUS_NO_DATA);
  assert(response_len == 0);
  assert(buffer[0] == '\0');
}

static void test_read_response_unknown_status_is_protocol_error(void) {
  static const uint8_t response[] = {42, 'x', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status;
  ezo_result_t result;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  ezo_fake_i2c_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_ERR_PROTOCOL);
  assert(status == EZO_STATUS_UNKNOWN);
  assert(response_len == 0);
}

static void test_read_response_requires_space_for_null_terminator(void) {
  static const uint8_t response[] = {1, '1', '2', '3', '4'};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status;
  ezo_result_t result;
  char buffer[4];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  ezo_fake_i2c_transport_set_response(&fake, response, sizeof(response));
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_ERR_BUFFER_TOO_SMALL);
  assert(status == EZO_STATUS_SUCCESS);
  assert(response_len == 0);
}

static void test_transport_failure_is_not_device_status(void) {
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status;
  ezo_result_t result;
  char buffer[8];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  fake.callback_result = EZO_ERR_TRANSPORT;
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device, buffer, sizeof(buffer), &response_len, &status);
  assert(result == EZO_ERR_TRANSPORT);
  assert(status == EZO_STATUS_UNKNOWN);
  assert(response_len == 0);
}

static void test_parse_double_rejects_trailing_garbage(void) {
  double value = 0.0;
  ezo_result_t result = ezo_parse_double("7.12x", 5, &value);
  assert(result == EZO_ERR_PARSE);
}

static void test_parse_double_accepts_signed_decimal_with_spaces(void) {
  double value = 0.0;
  ezo_result_t result = ezo_parse_double("  -12.50  ", 10, &value);
  assert(result == EZO_OK);
  assert(value < -12.49 && value > -12.51);
}

static void test_read_response_rejects_oversized_text_buffer(void) {
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  ezo_result_t result;
  char buffer[EZO_I2C_MAX_TEXT_RESPONSE_CAPACITY + 1];
  size_t response_len = 0;

  ezo_fake_i2c_transport_init(&fake);
  result = ezo_device_init(&device, 100, ezo_fake_i2c_transport_vtable(), &fake);
  assert(result == EZO_OK);

  result = ezo_read_response(&device,
                             buffer,
                             sizeof(buffer),
                             &response_len,
                             &status);
  assert(result == EZO_ERR_INVALID_ARGUMENT);
}

int main(void) {
  test_device_status_name_maps_public_statuses();
  test_send_command_records_bytes();
  test_send_command_with_float_formats_value();
  test_send_command_with_float_rounds_negative_values();
  test_send_command_with_float_rejects_excess_precision();
  test_send_command_with_float_rounds_halfway_decimal_up();
  test_send_command_with_float_rejects_non_finite_value();
  test_send_command_resets_last_device_status();
  test_read_response_success_and_parse();
  test_read_response_raw_preserves_embedded_zero_bytes();
  test_read_response_raw_detects_buffer_too_small();
  test_read_response_raw_not_ready_is_status_not_error();
  test_read_response_not_ready_is_status_not_error();
  test_read_response_no_data_is_status_not_error();
  test_read_response_unknown_status_is_protocol_error();
  test_read_response_requires_space_for_null_terminator();
  test_transport_failure_is_not_device_status();
  test_parse_double_rejects_trailing_garbage();
  test_parse_double_accepts_signed_decimal_with_spaces();
  test_read_response_rejects_oversized_text_buffer();
  return 0;
}
