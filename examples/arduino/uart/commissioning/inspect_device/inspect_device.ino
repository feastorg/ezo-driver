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

static void ensure_response_codes_enabled() {
  ezo_timing_hint_t hint;
  ezo_control_response_code_status_t response_code;

  fail_fast(ezo_uart_discard_input(&device));
  fail_fast(ezo_control_send_response_code_query_uart(&device, EZO_PRODUCT_UNKNOWN, &hint));
  delay(hint.wait_ms);
  fail_fast(ezo_control_read_response_code_uart(&device, &response_code));

#if EZO_UART_HAS_DEBUG_STREAM
  Serial.print("response_codes_before_bootstrap=");
  Serial.println((int)response_code.enabled);
#endif

  if (response_code.enabled != 0) {
    return;
  }

  fail_fast(ezo_control_send_response_code_set_uart(&device, EZO_PRODUCT_UNKNOWN, 1, &hint));
  delay(hint.wait_ms);
  fail_fast(ezo_uart_read_ok(&device));
}

static void inspect_once() {
  ezo_timing_hint_t hint;
  ezo_device_info_t info;
  ezo_control_status_t status;
  ensure_response_codes_enabled();

  fail_fast(ezo_control_send_info_query_uart(&device, EZO_PRODUCT_UNKNOWN, &hint));
  delay(hint.wait_ms);
  fail_fast(ezo_control_read_info_uart(&device, &info));

  fail_fast(ezo_control_send_status_query_uart(&device, info.product_id, &hint));
  delay(hint.wait_ms);
  fail_fast(ezo_control_read_status_uart(&device, &status));

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

  fail_fast(ezo_uart_arduino_stream_context_init(&uart_context, sensor_stream()));
  fail_fast(
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
