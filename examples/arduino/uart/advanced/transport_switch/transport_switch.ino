/*
Purpose: inspect UART transport state and optionally stage a software switch from UART mode to I2C mode.
Defaults: 9600 baud on the shared UART helper stream and target I2C address 99.
Assumptions: protocol lock is disabled before `APPLY_CHANGES` is enabled.
Next: reconnect over I2C and read ../../../i2c/commissioning/inspect_device/inspect_device.ino.
*/

#include <ezo_arduino_uart_example.hpp>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const uint8_t APPLY_CHANGES = 0U;
static const uint8_t TARGET_I2C_ADDRESS = 99U;

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t switch_done = 0U;

static void run_workflow() {
  ezo_device_info_t info;
  ezo_timing_hint_t hint;
  ezo_control_protocol_lock_status_t protocol_lock;

  ezo_arduino_uart_bootstrap_response_codes(&device, EZO_PRODUCT_UNKNOWN);

  EZO_ARDUINO_CHECK_OK("send_info_query",
                       ezo_control_send_info_query_uart(&device, EZO_PRODUCT_UNKNOWN, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_info_query", ezo_control_read_info_uart(&device, &info));

  EZO_ARDUINO_CHECK_OK("send_protocol_lock_query",
                       ezo_control_send_protocol_lock_query_uart(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_protocol_lock_query",
                       ezo_control_read_protocol_lock_uart(&device, &protocol_lock));

  Serial.println(F("transport=uart"));
  Serial.print(F("product_code="));
  Serial.println(info.product_code);
  Serial.print(F("firmware_version="));
  Serial.println(info.firmware_version);
  Serial.print(F("current_protocol_lock_enabled="));
  Serial.println((unsigned)protocol_lock.enabled);
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);
  Serial.println(F("planned_target_transport=i2c"));
  Serial.print(F("planned_target_address="));
  Serial.println((unsigned)TARGET_I2C_ADDRESS);

  if (APPLY_CHANGES == 0U) {
    return;
  }
  if (protocol_lock.enabled != 0U) {
    ezo_arduino_fail_fast("protocol_lock_enabled", EZO_ERR_PROTOCOL);
  }

  EZO_ARDUINO_CHECK_OK("switch_to_i2c",
                       ezo_control_send_switch_to_i2c_uart(
                           &device, info.product_id, TARGET_I2C_ADDRESS, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_switch_to_i2c_ack", ezo_uart_read_ok(&device));

  Serial.println(F("post_transport=i2c"));
  Serial.print(F("reconnect_i2c_address="));
  Serial.println((unsigned)TARGET_I2C_ADDRESS);
}

void setup() {
  ezo_arduino_uart_begin_streams(EZO_ARDUINO_UART_DEFAULT_BAUD);
  EZO_ARDUINO_CHECK_OK("init_uart_context", ezo_arduino_uart_init_context(&uart_context));
  EZO_ARDUINO_CHECK_OK("init_uart_device", ezo_arduino_uart_init_device(&device, &uart_context));
  startup_started_at_ms = millis();
}

void loop() {
  if (switch_done != 0U) {
    return;
  }

  if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
    return;
  }

  run_workflow();
  switch_done = 1U;
}
