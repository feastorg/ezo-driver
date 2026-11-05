#include "ezo_common.h"

#include <ctype.h>
#include <float.h>
#include <math.h>
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

static int ezo_common_is_finite(double value) {
  return value >= -DBL_MAX && value <= DBL_MAX;
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

  if (!ezo_common_is_finite(value)) {
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

  if (absolute_value > (((double)UINT64_MAX - 1.0) / (double)scale)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  scaled_value = absolute_value * (double)scale;
  if (!ezo_common_is_finite(scaled_value)) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  /*
   * Nudge the half-way boundary slightly to compensate for common binary
   * floating-point representation error in decimal command inputs such as
   * 1.005 or 2.675.
   */
  scaled_value += 0.5 + ((double)scale * DBL_EPSILON * 8.0);
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

/*
 * The digits are gathered into one integer mantissa and scaled by a single
 * multiplication or division by a power of ten. While the mantissa is at most
 * 2^53 and the power at most 10^22, both operands are exact doubles, so the
 * one IEEE-754 operation returns the nearest double to the decimal text. That
 * covers every reply up to 15 significant digits without needing strtod(),
 * which is not available on every embedded target. Where double is 32 bits,
 * as on AVR, the same holds for a mantissa up to 2^24 and a power up to 10^10.
 *
 * Digits beyond what a uint64_t holds still count toward the magnitude but no
 * longer toward the mantissa, so longer texts parse to a close but not
 * necessarily nearest value.
 */
ezo_result_t ezo_common_parse_double(const char *buffer,
                                     size_t buffer_len,
                                     double *value_out) {
  size_t i = 0;
  int sign = 1;
  int saw_digit = 0;
  uint64_t mantissa = 0;
  long exponent = 0;
  int mantissa_full = 0;
  double value = 0.0;
  double scale = 1.0;
  unsigned long scale_power = 0;

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
    if (!mantissa_full && mantissa <= (UINT64_MAX - 9u) / 10u) {
      mantissa = (mantissa * 10u) + (uint64_t)(buffer[i] - '0');
    } else {
      mantissa_full = 1;
      exponent += 1;
    }
    i += 1;
  }

  if (i < buffer_len && buffer[i] == '.') {
    i += 1;
    while (i < buffer_len && isdigit((unsigned char)buffer[i])) {
      saw_digit = 1;
      if (!mantissa_full && mantissa <= (UINT64_MAX - 9u) / 10u) {
        mantissa = (mantissa * 10u) + (uint64_t)(buffer[i] - '0');
        exponent -= 1;
      } else {
        mantissa_full = 1;
      }
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

  scale_power = (unsigned long)(exponent < 0 ? -exponent : exponent);
  while (scale_power > 0 && isfinite(scale)) {
    scale *= 10.0;
    scale_power -= 1;
  }

  value = (double)mantissa;
  if (exponent < 0) {
    value /= scale;
  } else {
    value *= scale;
  }

  if (sign < 0) {
    value = -value;
  }

  if (!isfinite(value)) {
    return EZO_ERR_PARSE;
  }

  *value_out = value;
  return EZO_OK;
}
