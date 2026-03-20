// UART read example with explicit wait timing.
// On boards without a second hardware serial port, this example uses `Serial` as the
// sensor stream and does not emit debug output.

#include <ezo.h>
#include <ezo_uart.h>
#include <ezo_uart_arduino_stream.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(HAVE_HWSERIAL1)
#define EZO_UART_HAS_DEBUG_STREAM 1
#else
#define EZO_UART_HAS_DEBUG_STREAM 0
#endif

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;

static Stream *sensor_stream() {
#if EZO_UART_HAS_DEBUG_STREAM
  return &Serial1;
#else
  return &Serial;
#endif
}

static void begin_streams() {
#if EZO_UART_HAS_DEBUG_STREAM
  Serial.begin(115200);
  Serial1.begin(9600);
#else
  Serial.begin(9600);
#endif
}

static void fail_fast(ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

#if EZO_UART_HAS_DEBUG_STREAM
  Serial.print("driver error: ");
  Serial.println((int)result);
#endif

  while (true) {
  }
}

static void request_reading() {
  ezo_timing_hint_t hint;
  fail_fast(ezo_uart_send_read(&device, &hint));
  delay(hint.wait_ms);
}

void setup() {
  begin_streams();

  fail_fast(ezo_uart_arduino_stream_context_init(&uart_context, sensor_stream()));
  fail_fast(
      ezo_uart_device_init(&device, ezo_uart_arduino_stream_transport(), &uart_context));
  fail_fast(ezo_uart_discard_input(&device));

  request_reading();
}

void loop() {
  char response[32];
  size_t response_len = 0;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  double value = 0.0;

  fail_fast(ezo_uart_read_response(&device,
                                   response,
                                   sizeof(response),
                                   &response_len,
                                   &kind));

#if EZO_UART_HAS_DEBUG_STREAM
  if (kind == EZO_UART_RESPONSE_DATA &&
      ezo_parse_double(response, response_len, &value) == EZO_OK) {
    Serial.print("reading: ");
    Serial.println(value, 3);
  } else {
    Serial.print("response: ");
    Serial.println(response);
  }
#endif

  delay(1000);
  request_reading();
}
