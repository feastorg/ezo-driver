#include "ezo_common.h"

#include <ctype.h>
#include <stdint.h>

enum {
  EZO_COMMON_MAX_FORMAT_DECIMALS = 9
};

static ezo_result_t ezo_common_append_char(char *buffer,
                                           size_t buffer_len,
                                           size_t *used,
                                           char value) {
  if (buffer == NULL || used == NULL || buffer_len == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (*used + 1 >= buffer_len) {
    return EZO_ERR_BUFFER_TOO_SMALL;
  }

  buffer[*used] = value;
  *used += 1;
  buffer[*used] = '\0';
  return EZO_OK;
}

static ezo_result_t ezo_common_append_cstr(char *buffer,
                                           size_t buffer_len,
                                           size_t *used,
                                           const char *value) {
  size_t i = 0;

  if (value == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  for (i = 0; value[i] != '\0'; ++i) {
    ezo_result_t result = ezo_common_append_char(buffer, buffer_len, used, value[i]);
    if (result != EZO_OK) {
      return result;
    }
  }

  return EZO_OK;
}

static ezo_result_t ezo_common_append_uint64(char *buffer,
                                             size_t buffer_len,
                                             size_t *used,
                                             uint64_t value,
                                             uint8_t min_digits) {
  char digits[32];
  uint8_t count = 0;
  uint8_t i = 0;

  if (min_digits > sizeof(digits)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  do {
    digits[count] = (char)('0' + (value % 10U));
    value /= 10U;
    count += 1;
  } while (value > 0 && count < sizeof(digits));

  while (count < min_digits) {
    digits[count] = '0';
    count += 1;
  }

  for (i = 0; i < count; ++i) {
    ezo_result_t result =
        ezo_common_append_char(buffer, buffer_len, used, digits[count - 1U - i]);
    if (result != EZO_OK) {
      return result;
    }
  }

  return EZO_OK;
}

ezo_result_t ezo_common_format_fixed_command(char *buffer,
                                             size_t buffer_len,
                                             const char *prefix,
                                             double value,
                                             uint8_t decimals) {
  uint64_t scale = 1;
  uint8_t i = 0;
  size_t used = 0;
  int is_negative = 0;
  double absolute_value = value;
  double scaled_value = 0.0;
  uint64_t scaled_integer = 0;
  uint64_t integer_part = 0;
  uint64_t fractional_part = 0;
  ezo_result_t result = EZO_OK;

  if (buffer == NULL || buffer_len == 0 || prefix == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (decimals > EZO_COMMON_MAX_FORMAT_DECIMALS) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  buffer[0] = '\0';

  result = ezo_common_append_cstr(buffer, buffer_len, &used, prefix);
  if (result != EZO_OK) {
    return result;
  }

  if (value < 0.0) {
    is_negative = 1;
    absolute_value = -value;
  }

  for (i = 0; i < decimals; ++i) {
    scale *= 10U;
  }

  if (absolute_value > ((double)UINT64_MAX / (double)scale)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  scaled_value = (absolute_value * (double)scale) + 0.5;
  scaled_integer = (uint64_t)scaled_value;
  integer_part = scaled_integer / scale;
  fractional_part = scaled_integer % scale;

  if (is_negative) {
    result = ezo_common_append_char(buffer, buffer_len, &used, '-');
    if (result != EZO_OK) {
      return result;
    }
  }

  result = ezo_common_append_uint64(buffer, buffer_len, &used, integer_part, 1);
  if (result != EZO_OK) {
    return result;
  }

  if (decimals == 0) {
    return EZO_OK;
  }

  result = ezo_common_append_char(buffer, buffer_len, &used, '.');
  if (result != EZO_OK) {
    return result;
  }

  return ezo_common_append_uint64(buffer, buffer_len, &used, fractional_part, decimals);
}

ezo_result_t ezo_common_parse_double(const char *buffer,
                                     size_t buffer_len,
                                     double *value_out) {
  size_t i = 0;
  int sign = 1;
  int saw_digit = 0;
  double value = 0.0;
  double fractional_scale = 0.1;

  if (buffer == NULL || value_out == NULL || buffer_len == 0) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  while (i < buffer_len && isspace((unsigned char)buffer[i])) {
    i += 1;
  }

  if (i < buffer_len && (buffer[i] == '+' || buffer[i] == '-')) {
    if (buffer[i] == '-') {
      sign = -1;
    }
    i += 1;
  }

  while (i < buffer_len && isdigit((unsigned char)buffer[i])) {
    saw_digit = 1;
    value = (value * 10.0) + (double)(buffer[i] - '0');
    i += 1;
  }

  if (i < buffer_len && buffer[i] == '.') {
    i += 1;
    while (i < buffer_len && isdigit((unsigned char)buffer[i])) {
      saw_digit = 1;
      value += (double)(buffer[i] - '0') * fractional_scale;
      fractional_scale *= 0.1;
      i += 1;
    }
  }

  if (!saw_digit) {
    return EZO_ERR_PARSE;
  }

  while (i < buffer_len) {
    if (!isspace((unsigned char)buffer[i])) {
      return EZO_ERR_PARSE;
    }
    i += 1;
  }

  if (sign < 0) {
    value = -value;
  }

  *value_out = value;
  return EZO_OK;
}
