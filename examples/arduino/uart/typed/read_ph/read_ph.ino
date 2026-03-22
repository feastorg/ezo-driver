/*
Purpose: simple Arduino UART pH read with explicit `*OK` bootstrap and ownership.
Defaults: 9600 baud on Serial1 when available, otherwise Serial.
Assumptions: the connected device is a pH circuit reachable over UART.
Next: read ../../commissioning/inspect_device/inspect_device.ino for bootstrap and identity checks.
*/

// UART read example with explicit response-code bootstrap and `*OK` ownership.
// On boards without a second hardware serial port, this example uses `Serial` as the
// sensor stream and does not emit debug output.

#include <ezo_control.h>
#include <ezo_ph.h>
#include <ezo_uart_arduino_stream.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(HAVE_HWSERIAL1)
#define EZO_UART_HAS_DEBUG_STREAM 1
#else
#define EZO_UART_HAS_DEBUG_STREAM 0
#endif

enum {
  STARTUP_SETTLE_MS = 1000U
};

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t read_requested = 0;

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
  fail_fast(ezo_ph_send_read_uart(&device, &hint));
  delay(hint.wait_ms);
}

static void ensure_response_codes_enabled() {
  ezo_timing_hint_t hint;
  ezo_control_response_code_status_t response_code;

  fail_fast(ezo_control_send_response_code_query_uart(&device, EZO_PRODUCT_UNKNOWN, &hint));
  delay(hint.wait_ms);
  fail_fast(ezo_control_read_response_code_uart(&device, &response_code));
  if (response_code.enabled != 0) {
    return;
  }

  fail_fast(ezo_control_send_response_code_set_uart(&device, EZO_PRODUCT_UNKNOWN, 1, &hint));
  delay(hint.wait_ms);
  fail_fast(ezo_uart_read_ok(&device));
}

void setup() {
  begin_streams();

  fail_fast(ezo_uart_arduino_stream_context_init(&uart_context, sensor_stream()));
  fail_fast(
      ezo_uart_device_init(&device, ezo_uart_arduino_stream_transport(), &uart_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_ph_reading_t reading;

  if (read_requested == 0U) {
    if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
      return;
    }

    fail_fast(ezo_uart_discard_input(&device));
    ensure_response_codes_enabled();
    request_reading();
    read_requested = 1U;
    return;
  }

  fail_fast(ezo_ph_read_response_uart(&device, &reading));

#if EZO_UART_HAS_DEBUG_STREAM
  Serial.print("ph=");
  Serial.println(reading.ph, 3);
#endif

  delay(1000);
  request_reading();
}
