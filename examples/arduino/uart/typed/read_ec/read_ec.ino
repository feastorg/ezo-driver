/*
Purpose: simple Arduino UART EC read with explicit response-code bootstrap and output-config ownership.
Defaults: 9600 baud on the shared UART helper stream.
Assumptions: the connected device is an EC circuit reachable over UART.
Next: read ../../advanced/control_workflow/control_workflow.ino for shared UART admin state.
*/

#include <ezo_arduino_uart_example.hpp>

#include <ezo_ec.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;
static ezo_ec_output_config_t output_config;
static unsigned long startup_started_at_ms = 0;
static uint8_t read_requested = 0U;

static void request_output_config() {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_output_query", ezo_ec_send_output_query_uart(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_output_query", ezo_ec_read_output_config_uart(&device, &output_config));
}

static void request_reading() {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_read", ezo_ec_send_read_uart(&device, &hint));
  ezo_arduino_wait_hint(&hint);
}

void setup() {
  ezo_arduino_uart_begin_streams(EZO_ARDUINO_UART_DEFAULT_BAUD);
  EZO_ARDUINO_CHECK_OK("init_uart_context", ezo_arduino_uart_init_context(&uart_context));
  EZO_ARDUINO_CHECK_OK("init_uart_device", ezo_arduino_uart_init_device(&device, &uart_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_ec_reading_t reading;

  if (read_requested == 0U) {
    if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
      return;
    }

    ezo_arduino_uart_bootstrap_response_codes(&device, EZO_PRODUCT_EC);
    request_output_config();
    request_reading();
    read_requested = 1U;
    return;
  }

  EZO_ARDUINO_CHECK_OK("read_response",
                       ezo_ec_read_response_uart(&device, output_config.enabled_mask, &reading));
  Serial.print(F("output_mask="));
  Serial.println((unsigned long)output_config.enabled_mask);
  ezo_arduino_print_ec_reading(F(""), &reading);

  delay(1000);
  request_reading();
}
