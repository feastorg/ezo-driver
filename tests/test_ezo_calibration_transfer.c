#include "ezo_calibration_transfer.h"
#include "tests/fakes/ezo_fake_i2c_transport.h"
#include "tests/fakes/ezo_fake_uart_transport.h"

#include <assert.h>
#include <string.h>

static void test_parse_and_build_helpers_cover_transfer_primitives(void) {
  char command[96];
  ezo_calibration_export_info_t info;

  assert(ezo_calibration_parse_export_info("10,120", strlen("10,120"), &info) == EZO_OK);
  assert(info.chunk_count == 10U);
  assert(info.byte_count == 120U);

  assert(ezo_calibration_build_import_command(command, sizeof(command), "AAAABBBB") == EZO_OK);
  assert(strcmp(command, "Import,AAAABBBB") == 0);
}

static void test_i2c_helpers_cover_export_and_import_flows(void) {
  static const uint8_t export_info_response[] = {1, '1', '0', ',', '1', '2', '0', 0};
  static const uint8_t export_chunk_response[] = {1, 'A', 'A', 'A', 'A', 0};
  static const uint8_t import_status_response[] = {1, 0};
  static const uint8_t import_pending_response[] = {1, '*', 'P', 'e', 'n', 'd', 'i', 'n', 'g', 0};
  ezo_fake_i2c_transport_t fake;
  ezo_i2c_device_t device;
  ezo_timing_hint_t hint;
  ezo_calibration_export_info_t info;
  ezo_calibration_import_result_t import_result;
  char chunk[16];
  size_t chunk_len = 0;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;

  ezo_fake_i2c_transport_init(&fake);
  assert(ezo_device_init(&device, 99, ezo_fake_i2c_transport_vtable(), &fake) == EZO_OK);

  ezo_fake_i2c_transport_set_response(&fake, export_info_response, sizeof(export_info_response));
  assert(ezo_calibration_send_export_info_query_i2c(&device, EZO_PRODUCT_PH, &hint) == EZO_OK);
  assert(hint.wait_ms == 300);
  assert(ezo_calibration_read_export_info_i2c(&device, &info) == EZO_OK);
  assert(info.chunk_count == 10U);
  assert(info.byte_count == 120U);

  ezo_fake_i2c_transport_set_response(&fake, export_chunk_response, sizeof(export_chunk_response));
  assert(ezo_calibration_send_export_next_i2c(&device, EZO_PRODUCT_PH, &hint) == EZO_OK);
  assert(ezo_calibration_read_export_chunk_i2c(&device, chunk, sizeof(chunk), &chunk_len) ==
         EZO_OK);
  assert(chunk_len == 4U);
  assert(memcmp(chunk, "AAAA", 4) == 0);

  assert(ezo_calibration_send_import_i2c(&device, EZO_PRODUCT_PH, "BBBB", &hint) == EZO_OK);
  assert(memcmp(fake.last_tx_bytes, "Import,BBBB", strlen("Import,BBBB")) == 0);
  ezo_fake_i2c_transport_set_response(&fake, import_status_response, sizeof(import_status_response));
  assert(ezo_calibration_read_import_status_i2c(&device, &status) == EZO_OK);
  assert(status == EZO_STATUS_SUCCESS);

  ezo_fake_i2c_transport_set_response(&fake,
                                  import_pending_response,
                                  sizeof(import_pending_response));
  assert(ezo_calibration_read_import_result_i2c(&device, &import_result) == EZO_OK);
  assert(import_result.device_status == EZO_STATUS_SUCCESS);
  assert(import_result.pending_reboot == 1);
}

static void test_uart_helpers_cover_export_sequence_and_import_result(void) {
  static const uint8_t export_info_response[] = {'1', '0', ',', '1', '2', '0', '\r',
                                                 '*', 'O', 'K', '\r'};
  static const uint8_t export_chunk_sequence[] = {'A', 'A', 'A', 'A', '\r',
                                                  '*', 'D', 'O', 'N', 'E', '\r'};
  static const uint8_t import_result_response[] = {'*', 'O', 'K', '\r'};
  ezo_fake_uart_transport_t fake;
  ezo_uart_device_t device;
  ezo_timing_hint_t hint;
  ezo_calibration_export_info_t info;
  char chunk[16];
  size_t chunk_len = 0;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;

  ezo_fake_uart_transport_init(&fake);
  assert(ezo_uart_device_init(&device, ezo_fake_uart_transport_vtable(), &fake) == EZO_OK);

  ezo_fake_uart_transport_set_response(&fake, export_info_response, sizeof(export_info_response));
  assert(ezo_calibration_send_export_info_query_uart(&device, EZO_PRODUCT_PH, &hint) == EZO_OK);
  assert(ezo_calibration_read_export_info_uart(&device, &info) == EZO_OK);
  assert(info.chunk_count == 10U);

  ezo_fake_uart_transport_set_response(&fake, export_chunk_sequence, sizeof(export_chunk_sequence));
  assert(ezo_calibration_send_export_next_uart(&device, EZO_PRODUCT_PH, &hint) == EZO_OK);
  assert(ezo_calibration_read_export_chunk_uart(&device,
                                                chunk,
                                                sizeof(chunk),
                                                &chunk_len,
                                                &kind) == EZO_OK);
  assert(kind == EZO_UART_RESPONSE_DATA);
  assert(chunk_len == 4U);
  assert(memcmp(chunk, "AAAA", 4) == 0);
  assert(ezo_calibration_read_export_chunk_uart(&device,
                                                chunk,
                                                sizeof(chunk),
                                                &chunk_len,
                                                &kind) == EZO_OK);
  assert(kind == EZO_UART_RESPONSE_DONE);

  assert(ezo_calibration_send_import_uart(&device, EZO_PRODUCT_PH, "BBBB", &hint) == EZO_OK);
  assert(memcmp(&fake.tx_bytes[fake.tx_len - strlen("Import,BBBB\r")],
                "Import,BBBB\r",
                strlen("Import,BBBB\r")) == 0);
  ezo_fake_uart_transport_set_response(&fake,
                                       import_result_response,
                                       sizeof(import_result_response));
  assert(ezo_calibration_read_import_result_uart(&device, &kind) == EZO_OK);
  assert(kind == EZO_UART_RESPONSE_OK);
}

int main(void) {
  test_parse_and_build_helpers_cover_transfer_primitives();
  test_i2c_helpers_cover_export_and_import_flows();
  test_uart_helpers_cover_export_sequence_and_import_result();
  return 0;
}