#include "ezo_parse.h"

#include <string.h>

static int ezo_parse_is_space(char value) {
  return value == ' ' || value == '\t' || value == '\n' || value == '\r' ||
         value == '\f' || value == '\v';
}

static void ezo_parse_trim_range(const char *buffer,
                                 size_t buffer_len,
                                 size_t *start_out,
                                 size_t *end_out) {
  size_t start = 0;
  size_t end = buffer_len;

  while (start < end && ezo_parse_is_space(buffer[start])) {
    start += 1;
  }

  while (end > start && ezo_parse_is_space(buffer[end - 1U])) {
    end -= 1;
  }

  *start_out = start;
  *end_out = end;
}

void ezo_text_span_clear(ezo_text_span_t *span) {
  if (span == NULL) {
    return;
  }

  span->text = NULL;
  span->length = 0;
}

int ezo_text_span_is_empty(ezo_text_span_t span) {
  return span.length == 0;
}

int ezo_text_span_equals_cstr(ezo_text_span_t span, const char *text) {
  size_t text_len = 0;

  if (text == NULL) {
    return 0;
  }

  text_len = strlen(text);
  if (span.length != text_len || span.text == NULL) {
    return 0;
  }

  return memcmp(span.text, text, text_len) == 0;
}

ezo_result_t ezo_parse_text_span_double(ezo_text_span_t span, double *value_out) {
  if (value_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (span.text == NULL || span.length == 0) {
    return EZO_ERR_PARSE;
  }

  return ezo_parse_double(span.text, span.length, value_out);
}

ezo_result_t ezo_parse_csv_fields(const char *buffer,
                                  size_t buffer_len,
                                  ezo_text_span_t *fields_out,
                                  size_t fields_capacity,
                                  size_t *field_count_out) {
  size_t start = 0;
  size_t end = 0;
  size_t field_count = 0;
  size_t segment_start = 0;
  size_t i = 0;

  if (buffer == NULL || buffer_len == 0 || fields_out == NULL || fields_capacity == 0 ||
      field_count_out == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  ezo_parse_trim_range(buffer, buffer_len, &start, &end);
  if (end <= start) {
    return EZO_ERR_PARSE;
  }

  for (i = 0; i < fields_capacity; ++i) {
    ezo_text_span_clear(&fields_out[i]);
  }

  segment_start = start;
  for (i = start; i <= end; ++i) {
    const int at_end = i == end;

    if (!at_end && buffer[i] != ',') {
      continue;
    }

    if (field_count >= fields_capacity) {
      return EZO_ERR_BUFFER_TOO_SMALL;
    }

    {
      size_t local_start = 0;
      size_t local_end = 0;
      size_t segment_len = i - segment_start;

      ezo_parse_trim_range(buffer + segment_start, segment_len, &local_start, &local_end);
      fields_out[field_count].text = buffer + segment_start + local_start;
      fields_out[field_count].length = local_end - local_start;
    }

    field_count += 1;
    segment_start = i + 1U;
  }

  *field_count_out = field_count;
  return EZO_OK;
}

ezo_result_t ezo_parse_query_response(const char *buffer,
                                      size_t buffer_len,
                                      ezo_text_span_t *prefix_out,
                                      ezo_text_span_t *fields_out,
                                      size_t fields_capacity,
                                      size_t *field_count_out) {
  size_t start = 0;
  size_t end = 0;
  size_t first_comma = 0;
  size_t i = 0;
  int saw_comma = 0;

  if (buffer == NULL || buffer_len == 0 || prefix_out == NULL || field_count_out == NULL ||
      (fields_capacity > 0 && fields_out == NULL)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  ezo_text_span_clear(prefix_out);
  if (fields_out != NULL) {
    for (i = 0; i < fields_capacity; ++i) {
      ezo_text_span_clear(&fields_out[i]);
    }
  }
  *field_count_out = 0;

  ezo_parse_trim_range(buffer, buffer_len, &start, &end);
  if (end <= start || buffer[start] != '?') {
    return EZO_ERR_PARSE;
  }

  first_comma = end;
  for (i = start; i < end; ++i) {
    if (buffer[i] == ',') {
      first_comma = i;
      saw_comma = 1;
      break;
    }
  }

  {
    size_t prefix_start = 0;
    size_t prefix_end = 0;
    size_t prefix_len = first_comma - start;

    ezo_parse_trim_range(buffer + start, prefix_len, &prefix_start, &prefix_end);
    prefix_out->text = buffer + start + prefix_start;
    prefix_out->length = prefix_end - prefix_start;
  }

  if (prefix_out->length == 0) {
    return EZO_ERR_PARSE;
  }

  if (!saw_comma) {
    return EZO_OK;
  }

  if (fields_out == NULL || fields_capacity == 0) {
    return EZO_ERR_BUFFER_TOO_SMALL;
  }

  return ezo_parse_csv_fields(buffer + first_comma + 1U,
                              end - first_comma - 1U,
                              fields_out,
                              fields_capacity,
                              field_count_out);
}

ezo_result_t ezo_parse_prefixed_fields(const char *buffer,
                                       size_t buffer_len,
                                       const char *prefix,
                                       ezo_text_span_t *fields_out,
                                       size_t fields_capacity,
                                       size_t *field_count_out) {
  ezo_text_span_t parsed_prefix;
  ezo_result_t result = EZO_OK;

  if (prefix == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_parse_query_response(buffer,
                                    buffer_len,
                                    &parsed_prefix,
                                    fields_out,
                                    fields_capacity,
                                    field_count_out);
  if (result != EZO_OK) {
    return result;
  }

  if (!ezo_text_span_equals_cstr(parsed_prefix, prefix)) {
    return EZO_ERR_PARSE;
  }

  return EZO_OK;
}

void ezo_uart_sequence_init(ezo_uart_sequence_t *sequence) {
  if (sequence == NULL) {
    return;
  }

  sequence->line_count = 0;
  sequence->data_line_count = 0;
  sequence->control_line_count = 0;
  sequence->last_kind = EZO_UART_RESPONSE_UNKNOWN;
  sequence->terminal_kind = EZO_UART_RESPONSE_UNKNOWN;
  sequence->complete = 0;
}

ezo_result_t ezo_uart_sequence_push_line(ezo_uart_sequence_t *sequence,
                                         ezo_uart_response_kind_t kind,
                                         ezo_uart_sequence_step_t *step_out) {
  ezo_uart_sequence_step_t step = EZO_UART_SEQUENCE_STEP_NONE;

  if (sequence == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (sequence->complete != 0) {
    return EZO_ERR_PROTOCOL;
  }

  switch (kind) {
  case EZO_UART_RESPONSE_DATA:
    sequence->data_line_count += 1;
    step = EZO_UART_SEQUENCE_STEP_DATA;
    break;
  case EZO_UART_RESPONSE_OK:
  case EZO_UART_RESPONSE_ERROR:
  case EZO_UART_RESPONSE_OVER_VOLTAGE:
  case EZO_UART_RESPONSE_UNDER_VOLTAGE:
  case EZO_UART_RESPONSE_RESET:
  case EZO_UART_RESPONSE_READY:
  case EZO_UART_RESPONSE_SLEEP:
  case EZO_UART_RESPONSE_WAKE:
  case EZO_UART_RESPONSE_DONE:
    sequence->control_line_count += 1;
    step = ezo_uart_response_kind_is_terminal(kind) ? EZO_UART_SEQUENCE_STEP_TERMINAL
                                                    : EZO_UART_SEQUENCE_STEP_CONTROL;
    break;
  case EZO_UART_RESPONSE_UNKNOWN:
  default:
    return EZO_ERR_INVALID_ARGUMENT;
  }

  sequence->line_count += 1;
  sequence->last_kind = kind;
  if (step == EZO_UART_SEQUENCE_STEP_TERMINAL) {
    sequence->terminal_kind = kind;
    sequence->complete = 1;
  }

  if (step_out != NULL) {
    *step_out = step;
  }

  return EZO_OK;
}

int ezo_uart_sequence_is_complete(const ezo_uart_sequence_t *sequence) {
  if (sequence == NULL) {
    return 0;
  }

  return sequence->complete != 0;
}
