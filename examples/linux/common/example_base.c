#include "example_base.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ezo_example_is_flag(const char *text) {
  return text != NULL && text[0] == '-';
}

int ezo_example_parse_uint32_arg(const char *text, uint32_t *value_out) {
  char *end = NULL;
  unsigned long value = 0;

  if (text == NULL || value_out == NULL || text[0] == '\0') {
    return 0;
  }

  value = strtoul(text, &end, 10);
  if (end == NULL || *end != '\0') {
    return 0;
  }

  *value_out = (uint32_t)value;
  return 1;
}

int ezo_example_parse_uint8_arg(const char *text, uint8_t *value_out) {
  uint32_t value = 0;

  if (!ezo_example_parse_uint32_arg(text, &value) || value > 127U) {
    return 0;
  }

  *value_out = (uint8_t)value;
  return 1;
}

int ezo_example_parse_baud_arg(const char *text,
                               ezo_uart_posix_baud_t *baud_out,
                               uint32_t *baud_rate_out) {
  uint32_t baud_rate = 0;

  if (!ezo_example_parse_uint32_arg(text, &baud_rate)) {
    return 0;
  }

  switch (baud_rate) {
    case 300U:
      *baud_out = EZO_UART_POSIX_BAUD_300;
      break;
    case 1200U:
      *baud_out = EZO_UART_POSIX_BAUD_1200;
      break;
    case 2400U:
      *baud_out = EZO_UART_POSIX_BAUD_2400;
      break;
    case 9600U:
      *baud_out = EZO_UART_POSIX_BAUD_9600;
      break;
    case 19200U:
      *baud_out = EZO_UART_POSIX_BAUD_19200;
      break;
    case 38400U:
      *baud_out = EZO_UART_POSIX_BAUD_38400;
      break;
    case 57600U:
      *baud_out = EZO_UART_POSIX_BAUD_57600;
      break;
    case 115200U:
      *baud_out = EZO_UART_POSIX_BAUD_115200;
      break;
    default:
      return 0;
  }

  *baud_rate_out = baud_rate;
  return 1;
}

static int ezo_example_resolve_default_baud(uint32_t default_baud_rate,
                                            ezo_uart_posix_baud_t *baud_out,
                                            uint32_t *baud_rate_out) {
  switch (default_baud_rate) {
    case 300U:
      *baud_out = EZO_UART_POSIX_BAUD_300;
      break;
    case 1200U:
      *baud_out = EZO_UART_POSIX_BAUD_1200;
      break;
    case 2400U:
      *baud_out = EZO_UART_POSIX_BAUD_2400;
      break;
    case 9600U:
      *baud_out = EZO_UART_POSIX_BAUD_9600;
      break;
    case 19200U:
      *baud_out = EZO_UART_POSIX_BAUD_19200;
      break;
    case 38400U:
      *baud_out = EZO_UART_POSIX_BAUD_38400;
      break;
    case 57600U:
      *baud_out = EZO_UART_POSIX_BAUD_57600;
      break;
    case 115200U:
      *baud_out = EZO_UART_POSIX_BAUD_115200;
      break;
    default:
      return 0;
  }

  *baud_rate_out = default_baud_rate;
  return 1;
}

int ezo_example_parse_i2c_options(int argc,
                                  char **argv,
                                  uint8_t default_address,
                                  ezo_example_i2c_options_t *options_out,
                                  int *next_arg_out) {
  int index = 1;

  if (options_out == NULL || next_arg_out == NULL) {
    return 0;
  }

  options_out->device_path = EZO_EXAMPLE_I2C_DEFAULT_PATH;
  options_out->address = default_address;

  if (argc > index && !ezo_example_is_flag(argv[index])) {
    uint8_t parsed_address = 0;

    if (ezo_example_parse_uint8_arg(argv[index], &parsed_address) &&
        (argc <= index + 1 || ezo_example_is_flag(argv[index + 1]))) {
      options_out->address = parsed_address;
      index += 1;
    } else {
      options_out->device_path = argv[index];
      index += 1;
    }
  }

  if (argc > index && !ezo_example_is_flag(argv[index])) {
    if (!ezo_example_parse_uint8_arg(argv[index], &options_out->address)) {
      return 0;
    }
    index += 1;
  }

  *next_arg_out = index;
  return 1;
}

int ezo_example_parse_uart_options(int argc,
                                   char **argv,
                                   uint32_t default_baud_rate,
                                   ezo_example_uart_options_t *options_out,
                                   int *next_arg_out) {
  int index = 1;

  if (options_out == NULL || next_arg_out == NULL) {
    return 0;
  }

  options_out->device_path = EZO_EXAMPLE_UART_DEFAULT_PATH;
  if (!ezo_example_resolve_default_baud(default_baud_rate,
                                        &options_out->baud,
                                        &options_out->baud_rate)) {
    return 0;
  }

  if (argc > index && !ezo_example_is_flag(argv[index])) {
    ezo_uart_posix_baud_t parsed_baud = EZO_UART_POSIX_BAUD_9600;
    uint32_t parsed_baud_rate = 0;

    if (ezo_example_parse_baud_arg(argv[index], &parsed_baud, &parsed_baud_rate) &&
        (argc <= index + 1 || ezo_example_is_flag(argv[index + 1]))) {
      options_out->baud = parsed_baud;
      options_out->baud_rate = parsed_baud_rate;
      index += 1;
    } else {
      options_out->device_path = argv[index];
      index += 1;
    }
  }

  if (argc > index && !ezo_example_is_flag(argv[index])) {
    if (!ezo_example_parse_baud_arg(argv[index], &options_out->baud, &options_out->baud_rate)) {
      return 0;
    }
    index += 1;
  }

  *next_arg_out = index;
  return 1;
}

int ezo_example_has_flag(int argc, char **argv, int start_index, const char *flag) {
  int index = 0;

  if (argv == NULL || flag == NULL) {
    return 0;
  }

  for (index = start_index; index < argc; ++index) {
    if (strcmp(argv[index], flag) == 0) {
      return 1;
    }
  }

  return 0;
}

const char *ezo_example_find_option_value(int argc,
                                          char **argv,
                                          int start_index,
                                          const char *prefix) {
  size_t prefix_len = 0;
  int index = 0;

  if (argv == NULL || prefix == NULL) {
    return NULL;
  }

  prefix_len = strlen(prefix);
  for (index = start_index; index < argc; ++index) {
    if (strncmp(argv[index], prefix, prefix_len) == 0) {
      return argv[index] + prefix_len;
    }
  }

  return NULL;
}

void ezo_example_wait_hint(const ezo_timing_hint_t *hint) {
  if (hint == NULL || hint->wait_ms == 0U) {
    return;
  }

  usleep((useconds_t)(hint->wait_ms * 1000U));
}

int ezo_example_print_error(const char *step, ezo_result_t result) {
  fprintf(stderr,
          "step=%s result=%s result_code=%d\n",
          step != NULL ? step : "unknown",
          ezo_result_name(result),
          (int)result);
  return 1;
}
