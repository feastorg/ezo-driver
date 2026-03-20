#ifndef EZO_PARSE_H
#define EZO_PARSE_H

#include "ezo.h"
#include "ezo_uart.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *text;
  size_t length;
} ezo_text_span_t;

typedef struct {
  size_t line_count;
  size_t data_line_count;
  size_t control_line_count;
  ezo_uart_response_kind_t last_kind;
  ezo_uart_response_kind_t terminal_kind;
  uint8_t complete;
} ezo_uart_sequence_t;

typedef enum {
  EZO_UART_SEQUENCE_STEP_NONE = 0,
  EZO_UART_SEQUENCE_STEP_DATA,
  EZO_UART_SEQUENCE_STEP_CONTROL,
  EZO_UART_SEQUENCE_STEP_TERMINAL
} ezo_uart_sequence_step_t;

void ezo_text_span_clear(ezo_text_span_t *span);

int ezo_text_span_is_empty(ezo_text_span_t span);

int ezo_text_span_equals_cstr(ezo_text_span_t span, const char *text);

ezo_result_t ezo_parse_text_span_double(ezo_text_span_t span, double *value_out);

ezo_result_t ezo_parse_csv_fields(const char *buffer,
                                  size_t buffer_len,
                                  ezo_text_span_t *fields_out,
                                  size_t fields_capacity,
                                  size_t *field_count_out);

ezo_result_t ezo_parse_query_response(const char *buffer,
                                      size_t buffer_len,
                                      ezo_text_span_t *prefix_out,
                                      ezo_text_span_t *fields_out,
                                      size_t fields_capacity,
                                      size_t *field_count_out);

ezo_result_t ezo_parse_prefixed_fields(const char *buffer,
                                       size_t buffer_len,
                                       const char *prefix,
                                       ezo_text_span_t *fields_out,
                                       size_t fields_capacity,
                                       size_t *field_count_out);

void ezo_uart_sequence_init(ezo_uart_sequence_t *sequence);

ezo_result_t ezo_uart_sequence_push_line(ezo_uart_sequence_t *sequence,
                                         ezo_uart_response_kind_t kind,
                                         ezo_uart_sequence_step_t *step_out);

int ezo_uart_sequence_is_complete(const ezo_uart_sequence_t *sequence);

#ifdef __cplusplus
}
#endif

#endif
