/*
Purpose: inspect shared UART control state and optionally apply name, LED, response-code, protocol-lock, find, sleep, or factory-reset operations.
Defaults: 9600 baud on the shared UART helper stream.
Assumptions: the device is reachable over UART and discrete-response mode can be bootstrapped.
Next: read ../transport_switch/transport_switch.ino for the software path that changes transport mode.
*/

#include <ezo_arduino_uart_example.hpp>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const uint8_t APPLY_CHANGES = 0U;
static const char *PLANNED_NAME = "tank";
static const int8_t PLANNED_LED = -1;
static const int8_t PLANNED_PROTOCOL_LOCK = -1;
static const int8_t PLANNED_RESPONSE_CODES = -1;
static const uint8_t SEND_FIND = 0U;
static const uint8_t SEND_SLEEP = 0U;
static const uint8_t SEND_FACTORY_RESET = 0U;

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t workflow_done = 0U;

static void run_workflow() {
  ezo_device_info_t info;
  ezo_timing_hint_t hint;
  ezo_control_name_t name;
  ezo_control_status_t status;
  ezo_control_led_status_t led;
  ezo_control_protocol_lock_status_t protocol_lock;
  ezo_control_response_code_status_t response_code;

  ezo_arduino_uart_bootstrap_response_codes(&device, EZO_PRODUCT_UNKNOWN);

  EZO_ARDUINO_CHECK_OK("send_info_query",
                       ezo_control_send_info_query_uart(&device, EZO_PRODUCT_UNKNOWN, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_info_query", ezo_control_read_info_uart(&device, &info));

  EZO_ARDUINO_CHECK_OK("send_name_query",
                       ezo_control_send_name_query_uart(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_name_query", ezo_control_read_name_uart(&device, &name));

  EZO_ARDUINO_CHECK_OK("send_status_query",
                       ezo_control_send_status_query_uart(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_status_query", ezo_control_read_status_uart(&device, &status));

  EZO_ARDUINO_CHECK_OK("send_led_query",
                       ezo_control_send_led_query_uart(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_led_query", ezo_control_read_led_uart(&device, &led));

  EZO_ARDUINO_CHECK_OK("send_protocol_lock_query",
                       ezo_control_send_protocol_lock_query_uart(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_protocol_lock_query",
                       ezo_control_read_protocol_lock_uart(&device, &protocol_lock));

  EZO_ARDUINO_CHECK_OK("send_response_code_query",
                       ezo_control_send_response_code_query_uart(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_response_code_query",
                       ezo_control_read_response_code_uart(&device, &response_code));

  Serial.println(F("transport=uart"));
  Serial.print(F("product_code="));
  Serial.println(info.product_code);
  Serial.print(F("firmware_version="));
  Serial.println(info.firmware_version);
  Serial.print(F("current_name="));
  Serial.println(name.name);
  Serial.print(F("current_restart_code="));
  Serial.println(status.restart_code);
  Serial.print(F("current_supply_voltage_v="));
  Serial.println(status.supply_voltage, 3);
  Serial.print(F("current_led_enabled="));
  Serial.println((unsigned)led.enabled);
  Serial.print(F("current_protocol_lock_enabled="));
  Serial.println((unsigned)protocol_lock.enabled);
  Serial.print(F("current_response_codes_enabled="));
  Serial.println((unsigned)response_code.enabled);
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);

  if (APPLY_CHANGES == 0U) {
    return;
  }

  if (PLANNED_NAME[0] != '\0') {
    EZO_ARDUINO_CHECK_OK("send_name_set",
                         ezo_control_send_name_set_uart(&device, info.product_id, PLANNED_NAME, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_name_set_ack", ezo_uart_read_ok(&device));
  }
  if (PLANNED_LED >= 0) {
    EZO_ARDUINO_CHECK_OK("send_led_set",
                         ezo_control_send_led_set_uart(&device, info.product_id, (uint8_t)PLANNED_LED, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_led_set_ack", ezo_uart_read_ok(&device));
  }
  if (PLANNED_PROTOCOL_LOCK >= 0) {
    EZO_ARDUINO_CHECK_OK("send_protocol_lock_set",
                         ezo_control_send_protocol_lock_set_uart(
                             &device, info.product_id, (uint8_t)PLANNED_PROTOCOL_LOCK, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_protocol_lock_set_ack", ezo_uart_read_ok(&device));
  }
  if (PLANNED_RESPONSE_CODES >= 0) {
    EZO_ARDUINO_CHECK_OK("send_response_code_set",
                         ezo_control_send_response_code_set_uart(
                             &device, info.product_id, (uint8_t)PLANNED_RESPONSE_CODES, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_response_code_set_ack", ezo_uart_read_ok(&device));
  }
  if (SEND_FIND != 0U) {
    EZO_ARDUINO_CHECK_OK("send_find",
                         ezo_control_send_find_uart(&device, info.product_id, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_find_ack", ezo_uart_read_ok(&device));
  }
  if (SEND_SLEEP != 0U) {
    EZO_ARDUINO_CHECK_OK("send_sleep",
                         ezo_control_send_sleep_uart(&device, info.product_id, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_sleep_ack", ezo_uart_read_ok(&device));
  }
  if (SEND_FACTORY_RESET != 0U) {
    EZO_ARDUINO_CHECK_OK("send_factory_reset",
                         ezo_control_send_factory_reset_uart(&device, info.product_id, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_factory_reset_ack", ezo_uart_read_ok(&device));
  }
}

void setup() {
  ezo_arduino_uart_begin_streams(EZO_ARDUINO_UART_DEFAULT_BAUD);
  EZO_ARDUINO_CHECK_OK("init_uart_context", ezo_arduino_uart_init_context(&uart_context));
  EZO_ARDUINO_CHECK_OK("init_uart_device", ezo_arduino_uart_init_device(&device, &uart_context));
  startup_started_at_ms = millis();
}

void loop() {
  if (workflow_done != 0U) {
    return;
  }

  if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
    return;
  }

  run_workflow();
  workflow_done = 1U;
}
