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

enum {
  STARTUP_SETTLE_MS = 1000U
};

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;
static ezo_do_output_config_t output_config;
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

static void fail_fast(const char *step, ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

#if EZO_UART_HAS_DEBUG_STREAM
  Serial.print("driver_error_step=");
  Serial.println(step);
  Serial.print("driver_error_name=");
  Serial.println(ezo_result_name(result));
  Serial.print("driver_error_code=");
  Serial.println((int)result);
#endif

  while (true) {
  }
}

#define CHECK_OK(step, expr) fail_fast(step, (expr))

static void request_output_config() {
  ezo_timing_hint_t hint;

  CHECK_OK("send_output_query", ezo_do_send_output_query_uart(&device, &hint));
  delay(hint.wait_ms);
  CHECK_OK("read_output_query", ezo_do_read_output_config_uart(&device, &output_config));
}

static void request_reading() {
  ezo_timing_hint_t hint;

  CHECK_OK("send_read", ezo_do_send_read_uart(&device, &hint));
  delay(hint.wait_ms);
}

static void ensure_response_codes_enabled() {
  ezo_timing_hint_t hint;
  ezo_control_response_code_status_t response_code;

  CHECK_OK("send_response_code_query",
           ezo_control_send_response_code_query_uart(&device, EZO_PRODUCT_UNKNOWN, &hint));
  delay(hint.wait_ms);
  CHECK_OK("read_response_code_query", ezo_control_read_response_code_uart(&device, &response_code));
  if (response_code.enabled != 0) {
    return;
  }

  CHECK_OK("send_response_code_set",
           ezo_control_send_response_code_set_uart(&device, EZO_PRODUCT_UNKNOWN, 1, &hint));
  delay(hint.wait_ms);
  CHECK_OK("read_response_code_ack", ezo_uart_read_ok(&device));
}

void setup() {
  begin_streams();

  CHECK_OK("init_uart_context", ezo_uart_arduino_stream_context_init(&uart_context, sensor_stream()));
  CHECK_OK("init_uart_device",
           ezo_uart_device_init(&device, ezo_uart_arduino_stream_transport(), &uart_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_do_reading_t reading;

  if (read_requested == 0U) {
    if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
      return;
    }

    CHECK_OK("discard_input", ezo_uart_discard_input(&device));
    ensure_response_codes_enabled();
    request_output_config();
    request_reading();
    read_requested = 1U;
    return;
  }

  CHECK_OK("read_response", ezo_do_read_response_uart(&device, output_config.enabled_mask, &reading));

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
