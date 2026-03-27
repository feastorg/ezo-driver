#ifndef EZO_ARDUINO_UART_EXAMPLE_HPP
#define EZO_ARDUINO_UART_EXAMPLE_HPP

#include <Arduino.h>

#include <ezo_arduino_common.hpp>
#include <ezo_uart_arduino_stream.h>

#if defined(ARDUINO_ARCH_AVR)
#include <SoftwareSerial.h>
#endif

static const uint32_t EZO_ARDUINO_UART_DEFAULT_BAUD = 9600UL;

#if defined(ARDUINO_ARCH_AVR)
static const uint8_t EZO_ARDUINO_UART_SENSOR_RX_PIN = 10U;
static const uint8_t EZO_ARDUINO_UART_SENSOR_TX_PIN = 11U;
static const uint8_t EZO_ARDUINO_UART_HAS_DEBUG_STREAM = 1U;
inline Stream *ezo_arduino_uart_sensor_stream() {
  static SoftwareSerial sensor_serial(EZO_ARDUINO_UART_SENSOR_RX_PIN,
                                      EZO_ARDUINO_UART_SENSOR_TX_PIN);
  return &sensor_serial;
}
inline void ezo_arduino_uart_begin_streams(uint32_t baud_rate) {
  Serial.begin(115200);
  static_cast<SoftwareSerial *>(ezo_arduino_uart_sensor_stream())->begin(baud_rate);
}
#elif defined(ARDUINO_ARCH_ESP32)
static const int EZO_ARDUINO_UART_SENSOR_RX_PIN = 16;
static const int EZO_ARDUINO_UART_SENSOR_TX_PIN = 17;
static const uint8_t EZO_ARDUINO_UART_HAS_DEBUG_STREAM = 1U;
inline Stream *ezo_arduino_uart_sensor_stream() {
  return &Serial1;
}
inline void ezo_arduino_uart_begin_streams(uint32_t baud_rate) {
  Serial.begin(115200);
  Serial1.begin(baud_rate, SERIAL_8N1, EZO_ARDUINO_UART_SENSOR_RX_PIN, EZO_ARDUINO_UART_SENSOR_TX_PIN);
}
#elif defined(HAVE_HWSERIAL1)
static const uint8_t EZO_ARDUINO_UART_HAS_DEBUG_STREAM = 1U;
inline Stream *ezo_arduino_uart_sensor_stream() {
  return &Serial1;
}
inline void ezo_arduino_uart_begin_streams(uint32_t baud_rate) {
  Serial.begin(115200);
  Serial1.begin(baud_rate);
}
#else
static const uint8_t EZO_ARDUINO_UART_HAS_DEBUG_STREAM = 0U;
inline Stream *ezo_arduino_uart_sensor_stream() {
  return &Serial;
}
inline void ezo_arduino_uart_begin_streams(uint32_t baud_rate) {
  Serial.begin(baud_rate);
}
#endif

inline ezo_result_t ezo_arduino_uart_init_context(ezo_uart_arduino_stream_context_t *context) {
  if (context == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }
  return ezo_uart_arduino_stream_context_init(context, ezo_arduino_uart_sensor_stream());
}

inline ezo_result_t ezo_arduino_uart_init_device(ezo_uart_device_t *device,
                                                 ezo_uart_arduino_stream_context_t *context) {
  if (device == NULL || context == NULL) {
    return EZO_ERR_INVALID_ARGUMENT;
  }
  return ezo_uart_device_init(device, ezo_uart_arduino_stream_transport(), context);
}

inline void ezo_arduino_uart_bootstrap_response_codes(ezo_uart_device_t *device,
                                                      ezo_product_id_t product_id) {
  ezo_timing_hint_t hint;
  ezo_control_response_code_status_t response_code;

  EZO_ARDUINO_CHECK_OK("discard_input", ezo_uart_discard_input(device));
  EZO_ARDUINO_CHECK_OK("send_response_code_query",
                       ezo_control_send_response_code_query_uart(device, product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_response_code_query",
                       ezo_control_read_response_code_uart(device, &response_code));
  if (response_code.enabled != 0U) {
    return;
  }

  EZO_ARDUINO_CHECK_OK("send_response_code_set",
                       ezo_control_send_response_code_set_uart(device, product_id, 1U, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_response_code_ack", ezo_uart_read_ok(device));
}

#endif
