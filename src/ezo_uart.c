#include "ezo_common.h"
#include "ezo_uart.h"

#include <stddef.h>
#include <string.h>

static ezo_result_t ezo_uart_validate_device(const ezo_uart_device_t *device) {
  if (device == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (device->transport == NULL || device->transport->write_bytes == NULL ||
      device->transport->read_bytes == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  return EZO_OK;
}

static ezo_result_t ezo_uart_validate_command(const char *command) {
  size_t i = 0;

  if (command == NULL || command[0] == '\0') {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  for (i = 0; command[i] != '\0'; ++i) {
    if (command[i] == '\r' || command[i] == '\n') {
      return EZO_ERR_INVALID_ARGUMENT;
    }
  }

  return EZO_OK;
}

static ezo_uart_response_kind_t ezo_uart_classify_response(const char *buffer,
                                                           size_t buffer_len) {
  if (buffer_len == 5 && memcmp(buffer, "*DONE", 5) == 0) {
    return EZO_UART_RESPONSE_DONE;
  }

  if (buffer_len == 3 && memcmp(buffer, "*OK", 3) == 0) {
    return EZO_UART_RESPONSE_OK;
  }

  if (buffer_len == 3 && memcmp(buffer, "*ER", 3) == 0) {
    return EZO_UART_RESPONSE_ERROR;
  }

  if (buffer_len == 3 && memcmp(buffer, "*OV", 3) == 0) {
    return EZO_UART_RESPONSE_OVER_VOLTAGE;
  }

  if (buffer_len == 3 && memcmp(buffer, "*UV", 3) == 0) {
    return EZO_UART_RESPONSE_UNDER_VOLTAGE;
  }

  if (buffer_len == 3 && memcmp(buffer, "*RS", 3) == 0) {
    return EZO_UART_RESPONSE_RESET;
  }

  if (buffer_len == 3 && memcmp(buffer, "*RE", 3) == 0) {
    return EZO_UART_RESPONSE_READY;
  }

  if (buffer_len == 3 && memcmp(buffer, "*SL", 3) == 0) {
    return EZO_UART_RESPONSE_SLEEP;
  }

  if (buffer_len == 3 && memcmp(buffer, "*WA", 3) == 0) {
    return EZO_UART_RESPONSE_WAKE;
  }

  return EZO_UART_RESPONSE_DATA;
}

static ezo_result_t ezo_uart_write_terminated_command(ezo_uart_device_t *device,
                                                      const char *command) {
  static const uint8_t terminator = '\r';
  size_t command_len = strlen(command);
  ezo_result_t result = EZO_OK;

  result = device->transport->write_bytes(device->transport_context,
                                          (const uint8_t *)command,
                                          command_len);
  if (result != EZO_OK) {
    return result;
  }

  result = device->transport->write_bytes(device->transport_context, &terminator, 1);
  if (result != EZO_OK) {
    return result;
  }

  device->last_response_kind = (uint8_t)EZO_UART_RESPONSE_UNKNOWN;
  return EZO_OK;
}

ezo_result_t ezo_uart_device_init(ezo_uart_device_t *device,
                                  const ezo_uart_transport_t *transport,
                                  void *transport_context) {
  if (device == NULL || transport == NULL || transport->write_bytes == NULL ||
      transport->read_bytes == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  device->transport = transport;
  device->transport_context = transport_context;
  device->last_response_kind = (uint8_t)EZO_UART_RESPONSE_UNKNOWN;
  return EZO_OK;
}

ezo_uart_response_kind_t ezo_uart_device_get_last_response_kind(
    const ezo_uart_device_t *device) {
  if (device == NULL) {
    return EZO_UART_RESPONSE_UNKNOWN;
  }

  return (ezo_uart_response_kind_t)device->last_response_kind;
}

ezo_result_t ezo_uart_send_command(ezo_uart_device_t *device,
                                   const char *command,
                                   ezo_command_kind_t kind,
                                   ezo_timing_hint_t *timing_hint) {
  ezo_result_t result = EZO_OK;
  ezo_timing_hint_t local_hint;

  result = ezo_uart_validate_device(device);
  if (result != EZO_OK) {
    return result;
  }

  result = ezo_uart_validate_command(command);
  if (result != EZO_OK) {
    return result;
  }

  result = ezo_get_timing_hint_for_command_kind(kind,
                                                timing_hint != NULL ? timing_hint : &local_hint);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_write_terminated_command(device, command);
}

ezo_result_t ezo_uart_send_command_with_float(ezo_uart_device_t *device,
                                              const char *prefix,
                                              double value,
                                              uint8_t decimals,
                                              ezo_command_kind_t kind,
                                              ezo_timing_hint_t *timing_hint) {
  char command[64];
  ezo_result_t result = EZO_OK;

  if (prefix == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = ezo_common_format_fixed_command(command, sizeof(command), prefix, value, decimals);
  if (result != EZO_OK) {
    return result;
  }

  return ezo_uart_send_command(device, command, kind, timing_hint);
}

ezo_result_t ezo_uart_send_read(ezo_uart_device_t *device,
                                ezo_timing_hint_t *timing_hint) {
  return ezo_uart_send_command(device, "r", EZO_COMMAND_READ, timing_hint);
}

ezo_result_t ezo_uart_send_read_with_temp_comp(ezo_uart_device_t *device,
                                               double temperature_c,
                                               uint8_t decimals,
                                               ezo_timing_hint_t *timing_hint) {
  return ezo_uart_send_command_with_float(device,
                                          "rt,",
                                          temperature_c,
                                          decimals,
                                          EZO_COMMAND_READ_WITH_TEMP_COMP,
                                          timing_hint);
}

ezo_result_t ezo_uart_read_line(ezo_uart_device_t *device,
                                char *buffer,
                                size_t buffer_len,
                                size_t *response_len,
                                ezo_uart_response_kind_t *response_kind) {
  size_t used = 0;
  ezo_result_t result = EZO_OK;

  result = ezo_uart_validate_device(device);
  if (result != EZO_OK) {
    return result;
  }

  if (buffer == NULL || buffer_len == 0 || response_len == NULL || response_kind == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  if (buffer_len > (size_t)EZO_UART_MAX_TEXT_RESPONSE_CAPACITY) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  buffer[0] = '\0';
  *response_len = 0;
  *response_kind = EZO_UART_RESPONSE_UNKNOWN;
  device->last_response_kind = (uint8_t)EZO_UART_RESPONSE_UNKNOWN;

  for (;;) {
    uint8_t byte = 0;
    size_t rx_received = 0;

    result = device->transport->read_bytes(device->transport_context,
                                           &byte,
                                           1,
                                           &rx_received);
    if (result != EZO_OK) {
      return result;
    }

    if (rx_received == 0) {
      return EZO_ERR_PROTOCOL;
    }

    if (byte == '\r') {
      if (used == 0) {
        return EZO_ERR_PROTOCOL;
      }
      break;
    }

    if (used + 1 >= buffer_len) {
      return EZO_ERR_BUFFER_TOO_SMALL;
    }

    buffer[used] = (char)byte;
    used += 1;
    buffer[used] = '\0';
  }

  *response_len = used;
  *response_kind = ezo_uart_classify_response(buffer, used);
  device->last_response_kind = (uint8_t)(*response_kind);
  return EZO_OK;
}

int ezo_uart_response_kind_is_control(ezo_uart_response_kind_t response_kind) {
  switch (response_kind) {
  case EZO_UART_RESPONSE_OK:
  case EZO_UART_RESPONSE_ERROR:
  case EZO_UART_RESPONSE_OVER_VOLTAGE:
  case EZO_UART_RESPONSE_UNDER_VOLTAGE:
  case EZO_UART_RESPONSE_RESET:
  case EZO_UART_RESPONSE_READY:
  case EZO_UART_RESPONSE_SLEEP:
  case EZO_UART_RESPONSE_WAKE:
  case EZO_UART_RESPONSE_DONE:
    return 1;
  case EZO_UART_RESPONSE_UNKNOWN:
  case EZO_UART_RESPONSE_DATA:
  default:
    return 0;
  }
}

int ezo_uart_response_kind_is_terminal(ezo_uart_response_kind_t response_kind) {
  switch (response_kind) {
  case EZO_UART_RESPONSE_OK:
  case EZO_UART_RESPONSE_ERROR:
  case EZO_UART_RESPONSE_DONE:
    return 1;
  case EZO_UART_RESPONSE_UNKNOWN:
  case EZO_UART_RESPONSE_DATA:
  case EZO_UART_RESPONSE_OVER_VOLTAGE:
  case EZO_UART_RESPONSE_UNDER_VOLTAGE:
  case EZO_UART_RESPONSE_RESET:
  case EZO_UART_RESPONSE_READY:
  case EZO_UART_RESPONSE_SLEEP:
  case EZO_UART_RESPONSE_WAKE:
  default:
    return 0;
  }
}

ezo_result_t ezo_uart_discard_input(ezo_uart_device_t *device) {
  ezo_result_t result = ezo_uart_validate_device(device);
  if (result != EZO_OK) {
    return result;
  }

  if (device->transport->discard_input == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }

  result = device->transport->discard_input(device->transport_context);
  if (result == EZO_OK) {
    device->last_response_kind = (uint8_t)EZO_UART_RESPONSE_UNKNOWN;
  }

  return result;
}
