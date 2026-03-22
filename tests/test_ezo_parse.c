#include "ezo.h"
#include "ezo_parse.h"
#include "ezo_product.h"
#include "ezo_schema.h"

#include <assert.h>
#include <string.h>

static void test_parse_csv_fields_trims_and_preserves_empty_fields(void) {
  static const char payload[] = " 12.30 ,   , -4.50 ";
  ezo_text_span_t fields[3];
  size_t field_count = 0;
  double value = 0.0;

  assert(ezo_parse_csv_fields(payload, strlen(payload), fields, 3, &field_count) == EZO_OK);
  assert(field_count == 3);
  assert(ezo_text_span_equals_cstr(fields[0], "12.30") == 1);
  assert(ezo_text_span_is_empty(fields[1]) == 1);
  assert(ezo_text_span_equals_cstr(fields[2], "-4.50") == 1);
  assert(ezo_parse_text_span_double(fields[0], &value) == EZO_OK);
  assert(value > 12.29 && value < 12.31);
}

static void test_result_name_maps_public_results(void) {
  assert(strcmp(ezo_result_name(EZO_OK), "EZO_OK") == 0);
  assert(strcmp(ezo_result_name(EZO_ERR_INVALID_ARGUMENT), "EZO_ERR_INVALID_ARGUMENT") == 0);
  assert(strcmp(ezo_result_name(EZO_ERR_BUFFER_TOO_SMALL), "EZO_ERR_BUFFER_TOO_SMALL") == 0);
  assert(strcmp(ezo_result_name(EZO_ERR_TRANSPORT), "EZO_ERR_TRANSPORT") == 0);
  assert(strcmp(ezo_result_name(EZO_ERR_PROTOCOL), "EZO_ERR_PROTOCOL") == 0);
  assert(strcmp(ezo_result_name(EZO_ERR_PARSE), "EZO_ERR_PARSE") == 0);
  assert(strcmp(ezo_result_name((ezo_result_t)99), "EZO_ERR_UNKNOWN") == 0);
}

static void test_parse_query_response_extracts_prefix_and_fields(void) {
  static const char response[] = " ?Status, P , 5.038 ";
  ezo_text_span_t prefix;
  ezo_text_span_t fields[2];
  size_t field_count = 0;

  assert(ezo_parse_query_response(response, strlen(response), &prefix, fields, 2, &field_count) ==
         EZO_OK);
  assert(ezo_text_span_equals_cstr(prefix, "?Status") == 1);
  assert(field_count == 2);
  assert(ezo_text_span_equals_cstr(fields[0], "P") == 1);
  assert(ezo_text_span_equals_cstr(fields[1], "5.038") == 1);
}

static void test_parse_query_response_allows_prefix_only(void) {
  static const char response[] = "?Sleep";
  ezo_text_span_t prefix;
  size_t field_count = 0;

  assert(ezo_parse_query_response(response, strlen(response), &prefix, NULL, 0, &field_count) ==
         EZO_OK);
  assert(ezo_text_span_equals_cstr(prefix, "?Sleep") == 1);
  assert(field_count == 0);
}

static void test_parse_text_span_uint32_reads_decimal_tokens(void) {
  ezo_text_span_t value_span = {"225", 3};
  uint32_t value = 0;

  assert(ezo_parse_text_span_uint32(value_span, &value) == EZO_OK);
  assert(value == 225U);
}

static void test_parse_prefixed_fields_rejects_wrong_prefix(void) {
  static const char response[] = "?Name,my-pH";
  ezo_text_span_t fields[1];
  size_t field_count = 0;

  assert(ezo_parse_prefixed_fields(response,
                                   strlen(response),
                                   "?Status",
                                   fields,
                                   1,
                                   &field_count) == EZO_ERR_PARSE);
}

static void test_uart_sequence_tracks_data_and_terminal_status(void) {
  ezo_uart_sequence_t sequence;
  ezo_uart_sequence_step_t step = EZO_UART_SEQUENCE_STEP_NONE;

  ezo_uart_sequence_init(&sequence);
  assert(ezo_uart_sequence_push_line(&sequence, EZO_UART_RESPONSE_DATA, &step) == EZO_OK);
  assert(step == EZO_UART_SEQUENCE_STEP_DATA);
  assert(sequence.line_count == 1);
  assert(sequence.data_line_count == 1);
  assert(sequence.control_line_count == 0);
  assert(ezo_uart_sequence_is_complete(&sequence) == 0);

  assert(ezo_uart_sequence_push_line(&sequence, EZO_UART_RESPONSE_OK, &step) == EZO_OK);
  assert(step == EZO_UART_SEQUENCE_STEP_TERMINAL);
  assert(sequence.line_count == 2);
  assert(sequence.data_line_count == 1);
  assert(sequence.control_line_count == 1);
  assert(sequence.terminal_kind == EZO_UART_RESPONSE_OK);
  assert(ezo_uart_sequence_is_complete(&sequence) == 1);

  assert(ezo_uart_sequence_push_line(&sequence, EZO_UART_RESPONSE_DATA, NULL) ==
         EZO_ERR_PROTOCOL);
}

static void test_uart_sequence_tracks_nonterminal_control_tokens(void) {
  ezo_uart_sequence_t sequence;

  ezo_uart_sequence_init(&sequence);
  assert(ezo_uart_sequence_push_line(&sequence, EZO_UART_RESPONSE_WAKE, NULL) == EZO_OK);
  assert(ezo_uart_sequence_push_line(&sequence, EZO_UART_RESPONSE_READY, NULL) == EZO_OK);
  assert(ezo_uart_sequence_push_line(&sequence, EZO_UART_RESPONSE_DATA, NULL) == EZO_OK);
  assert(ezo_uart_sequence_push_line(&sequence, EZO_UART_RESPONSE_DONE, NULL) == EZO_OK);
  assert(sequence.line_count == 4);
  assert(sequence.data_line_count == 1);
  assert(sequence.control_line_count == 3);
  assert(sequence.terminal_kind == EZO_UART_RESPONSE_DONE);
  assert(ezo_uart_sequence_is_complete(&sequence) == 1);
}

static void test_schema_get_output_schema_uses_canonical_field_order(void) {
  ezo_output_schema_t ec_schema;
  ezo_output_schema_t hum_schema;

  assert(ezo_schema_get_output_schema(EZO_PRODUCT_EC, &ec_schema) == EZO_OK);
  assert(ec_schema.field_count == 4);
  assert(ec_schema.fields[0] == EZO_MEASUREMENT_FIELD_CONDUCTIVITY);
  assert(ec_schema.fields[1] == EZO_MEASUREMENT_FIELD_TOTAL_DISSOLVED_SOLIDS);
  assert(ec_schema.fields[2] == EZO_MEASUREMENT_FIELD_SALINITY);
  assert(ec_schema.fields[3] == EZO_MEASUREMENT_FIELD_SPECIFIC_GRAVITY);

  assert(ezo_schema_get_output_schema(EZO_PRODUCT_HUM, &hum_schema) == EZO_OK);
  assert(hum_schema.field_count == 3);
  assert(hum_schema.fields[0] == EZO_MEASUREMENT_FIELD_RELATIVE_HUMIDITY);
  assert(hum_schema.fields[1] == EZO_MEASUREMENT_FIELD_AIR_TEMPERATURE);
  assert(hum_schema.fields[2] == EZO_MEASUREMENT_FIELD_DEW_POINT);
}

static void test_schema_parse_scalar_reading(void) {
  ezo_scalar_reading_t reading;

  assert(ezo_schema_parse_scalar_reading(" -245.7 ", 8, EZO_MEASUREMENT_FIELD_ORP, &reading) ==
         EZO_OK);
  assert(reading.field == EZO_MEASUREMENT_FIELD_ORP);
  assert(reading.present == 1);
  assert(reading.value > -245.8 && reading.value < -245.6);
}

static void test_schema_parse_multi_output_reading_with_sparse_mask(void) {
  ezo_output_schema_t schema;
  ezo_multi_output_reading_t reading;

  assert(ezo_schema_get_output_schema(EZO_PRODUCT_EC, &schema) == EZO_OK);
  assert(ezo_schema_count_enabled_fields(&schema, 0x5u) == 2);
  assert(ezo_schema_parse_multi_output_reading("100.00,1.234",
                                               strlen("100.00,1.234"),
                                               &schema,
                                               0x5u,
                                               &reading) == EZO_OK);
  assert(reading.product_id == EZO_PRODUCT_EC);
  assert(reading.field_count == 4);
  assert(reading.present_mask == 0x5u);
  assert(reading.values[0].present == 1);
  assert(reading.values[0].field == EZO_MEASUREMENT_FIELD_CONDUCTIVITY);
  assert(reading.values[0].value > 99.99 && reading.values[0].value < 100.01);
  assert(reading.values[1].present == 0);
  assert(reading.values[2].present == 1);
  assert(reading.values[2].field == EZO_MEASUREMENT_FIELD_SALINITY);
  assert(reading.values[2].value > 1.233 && reading.values[2].value < 1.235);
  assert(reading.values[3].present == 0);
}

static void test_schema_parse_multi_output_reading_rejects_field_count_mismatch(void) {
  ezo_output_schema_t schema;
  ezo_multi_output_reading_t reading;

  assert(ezo_schema_get_output_schema(EZO_PRODUCT_DO, &schema) == EZO_OK);
  assert(ezo_schema_parse_multi_output_reading("8.42",
                                               strlen("8.42"),
                                               &schema,
                                               0x3u,
                                               &reading) == EZO_ERR_PARSE);
}

static void test_product_resolve_timing_hint_prefers_metadata_and_falls_back(void) {
  ezo_timing_hint_t hint;

  assert(ezo_product_resolve_timing_hint(EZO_PRODUCT_RTD,
                                         EZO_PRODUCT_TRANSPORT_I2C,
                                         EZO_COMMAND_READ,
                                         &hint) == EZO_OK);
  assert(hint.wait_ms == 600);

  assert(ezo_product_resolve_timing_hint(EZO_PRODUCT_UNKNOWN,
                                         EZO_PRODUCT_TRANSPORT_I2C,
                                         EZO_COMMAND_READ,
                                         &hint) == EZO_OK);
  assert(hint.wait_ms == 1000);

  assert(ezo_product_resolve_timing_hint(EZO_PRODUCT_ORP,
                                         EZO_PRODUCT_TRANSPORT_UART,
                                         EZO_COMMAND_READ_WITH_TEMP_COMP,
                                         &hint) == EZO_OK);
  assert(hint.wait_ms == 1000);
}

int main(void) {
  test_result_name_maps_public_results();
  test_parse_csv_fields_trims_and_preserves_empty_fields();
  test_parse_query_response_extracts_prefix_and_fields();
  test_parse_query_response_allows_prefix_only();
  test_parse_text_span_uint32_reads_decimal_tokens();
  test_parse_prefixed_fields_rejects_wrong_prefix();
  test_uart_sequence_tracks_data_and_terminal_status();
  test_uart_sequence_tracks_nonterminal_control_tokens();
  test_schema_get_output_schema_uses_canonical_field_order();
  test_schema_parse_scalar_reading();
  test_schema_parse_multi_output_reading_with_sparse_mask();
  test_schema_parse_multi_output_reading_rejects_field_count_mismatch();
  test_product_resolve_timing_hint_prefers_metadata_and_falls_back();
  return 0;
}
