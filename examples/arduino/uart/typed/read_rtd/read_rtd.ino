/*
Purpose: simple Arduino UART RTD read with explicit response-code bootstrap and scale query.
Defaults: 9600 baud on the shared UART helper stream.
Assumptions: the connected device is an RTD circuit reachable over UART.
Next: read ../../advanced/control_workflow/control_workflow.ino for shared UART admin state.
*/

#include <ezo_arduino_uart_example.hpp>

#include <ezo_rtd.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;
static ezo_rtd_scale_status_t scale_status;
static unsigned long startup_started_at_ms = 0;
static uint8_t read_requested = 0U;

static void request_scale() {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_scale_query", ezo_rtd_send_scale_query_uart(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_scale_query", ezo_rtd_read_scale_uart(&device, &scale_status));
}

static void request_reading() {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_read", ezo_rtd_send_read_uart(&device, &hint));
  ezo_arduino_wait_hint(&hint);
}

void setup() {
  ezo_arduino_uart_begin_streams(EZO_ARDUINO_UART_DEFAULT_BAUD);
  EZO_ARDUINO_CHECK_OK("init_uart_context", ezo_arduino_uart_init_context(&uart_context));
  EZO_ARDUINO_CHECK_OK("init_uart_device", ezo_arduino_uart_init_device(&device, &uart_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_rtd_reading_t reading;

  if (read_requested == 0U) {
    if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
      return;
    }

    ezo_arduino_uart_bootstrap_response_codes(&device, EZO_PRODUCT_RTD);
    request_scale();
    request_reading();
    read_requested = 1U;
    return;
  }

  EZO_ARDUINO_CHECK_OK("read_response",
                       ezo_rtd_read_response_uart(&device, scale_status.scale, &reading));
  Serial.print(F("scale="));
  Serial.println(ezo_arduino_rtd_scale_name(reading.scale));
  Serial.print(F("temperature="));
  Serial.println(reading.temperature, 3);

  delay(1000);
  request_reading();
}
