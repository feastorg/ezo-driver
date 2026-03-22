/*
Purpose: inspect an Arduino UART device and print bootstrap, identity, and status information.
Defaults: 9600 baud on Serial1 when available, otherwise Serial.
Assumptions: the sensor UART is wired to the chosen stream.
Next: read ../../typed/read_ph/read_ph.ino for the smallest typed UART read path.
*/

#include <ezo_control.h>
#include <ezo_uart.h>
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
static uint8_t inspection_done = 0;

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

static void ensure_response_codes_enabled() {
  ezo_timing_hint_t hint;
  ezo_control_response_code_status_t response_code;

  CHECK_OK("discard_input", ezo_uart_discard_input(&device));
  CHECK_OK("send_response_code_query",
           ezo_control_send_response_code_query_uart(&device, EZO_PRODUCT_UNKNOWN, &hint));
  delay(hint.wait_ms);
  CHECK_OK("read_response_code_query", ezo_control_read_response_code_uart(&device, &response_code));

#if EZO_UART_HAS_DEBUG_STREAM
  Serial.print("response_codes_before_bootstrap=");
  Serial.println((int)response_code.enabled);
#endif

  if (response_code.enabled != 0) {
    return;
  }

  CHECK_OK("send_response_code_set",
           ezo_control_send_response_code_set_uart(&device, EZO_PRODUCT_UNKNOWN, 1, &hint));
  delay(hint.wait_ms);
  CHECK_OK("read_response_code_ack", ezo_uart_read_ok(&device));
}

static void inspect_once() {
  ezo_timing_hint_t hint;
  ezo_device_info_t info;
  ezo_control_status_t status;
  ensure_response_codes_enabled();

  CHECK_OK("send_info_query",
           ezo_control_send_info_query_uart(&device, EZO_PRODUCT_UNKNOWN, &hint));
  delay(hint.wait_ms);
  CHECK_OK("read_info_query", ezo_control_read_info_uart(&device, &info));

  CHECK_OK("send_status_query",
           ezo_control_send_status_query_uart(&device, info.product_id, &hint));
  delay(hint.wait_ms);
  CHECK_OK("read_status_query", ezo_control_read_status_uart(&device, &status));

#if EZO_UART_HAS_DEBUG_STREAM
  Serial.print("product_code=");
  Serial.println(info.product_code);
  Serial.print("firmware_version=");
  Serial.println(info.firmware_version);
  Serial.print("restart_code=");
  Serial.println(status.restart_code);
  Serial.print("supply_voltage_v=");
  Serial.println(status.supply_voltage, 3);
#endif
}

void setup() {
  begin_streams();

  CHECK_OK("init_uart_context", ezo_uart_arduino_stream_context_init(&uart_context, sensor_stream()));
  CHECK_OK("init_uart_device",
           ezo_uart_device_init(&device, ezo_uart_arduino_stream_transport(), &uart_context));
  startup_started_at_ms = millis();
}

void loop() {
  if (inspection_done != 0U) {
    return;
  }

  if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
    return;
  }

  inspect_once();
  inspection_done = 1U;
}
