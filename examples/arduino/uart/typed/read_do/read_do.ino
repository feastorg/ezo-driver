/*
Purpose: simple Arduino UART dissolved-oxygen read with explicit `*OK` bootstrap and output-config ownership.
Defaults: 9600 baud on Serial1 when available, otherwise Serial.
Assumptions: the connected device is a D.O. circuit reachable over UART.
Next: read ../../commissioning/inspect_device/inspect_device.ino for bootstrap and identity checks.
*/

// UART read example with explicit response-code bootstrap and `*OK` ownership.
// On boards without a second hardware serial port, this example uses `Serial` as the
// sensor stream and does not emit debug output.

#include <ezo_control.h>
#include <ezo_do.h>
#include <ezo_uart_arduino_stream.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(HAVE_HWSERIAL1)
#define EZO_UART_HAS_DEBUG_STREAM 1
#else
#define EZO_UART_HAS_DEBUG_STREAM 0
#endif

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;
static ezo_do_output_config_t output_config;

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
  Serial.print("driver_error=");
  Serial.println((int)result);
#endif

  while (true) {
  }
}

static void request_output_config() {
  ezo_timing_hint_t hint;

  fail_fast(ezo_do_send_output_query_uart(&device, &hint));
  delay(hint.wait_ms);
  fail_fast(ezo_do_read_output_config_uart(&device, &output_config));
}

static void request_reading() {
  ezo_timing_hint_t hint;

  fail_fast(ezo_do_send_read_uart(&device, &hint));
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
  fail_fast(ezo_uart_discard_input(&device));
  ensure_response_codes_enabled();
  request_output_config();
  request_reading();
}

void loop() {
  ezo_do_reading_t reading;

  fail_fast(ezo_do_read_response_uart(&device, output_config.enabled_mask, &reading));

#if EZO_UART_HAS_DEBUG_STREAM
  Serial.print("output_mask=");
  Serial.println((unsigned long)output_config.enabled_mask);
  if ((reading.present_mask & EZO_DO_OUTPUT_MG_L) != 0U) {
    Serial.print("milligrams_per_liter=");
    Serial.println(reading.milligrams_per_liter, 3);
  }
  if ((reading.present_mask & EZO_DO_OUTPUT_PERCENT_SATURATION) != 0U) {
    Serial.print("percent_saturation=");
    Serial.println(reading.percent_saturation, 3);
  }
#endif

  delay(1000);
  request_reading();
}
