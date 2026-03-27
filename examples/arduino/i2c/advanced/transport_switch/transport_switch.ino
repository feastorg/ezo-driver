/*
Purpose: inspect I2C transport state and optionally stage a software switch from I2C mode to UART mode.
Defaults: Wire on the board default pins, address 99, and target UART baud 9600.
Assumptions: protocol lock is disabled before `APPLY_CHANGES` is enabled.
Next: reconnect over UART and read ../../../uart/commissioning/inspect_device/inspect_device.ino.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_control.h>

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 99U;
static const uint8_t APPLY_CHANGES = 0U;
static const uint32_t TARGET_UART_BAUD = 9600UL;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t switch_done = 0U;

static void run_workflow() {
  ezo_device_info_t info;
  ezo_timing_hint_t hint;
  ezo_control_protocol_lock_status_t protocol_lock;

  ezo_arduino_i2c_drain_pending(&device);
  EZO_ARDUINO_CHECK_OK("send_info_query",
                       ezo_control_send_info_query_i2c(&device, EZO_PRODUCT_UNKNOWN, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_info_query", ezo_control_read_info_i2c(&device, &info));

  EZO_ARDUINO_CHECK_OK("send_protocol_lock_query",
                       ezo_control_send_protocol_lock_query_i2c(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_protocol_lock_query",
                       ezo_control_read_protocol_lock_i2c(&device, &protocol_lock));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("product_code="));
  Serial.println(info.product_code);
  Serial.print(F("firmware_version="));
  Serial.println(info.firmware_version);
  Serial.print(F("current_protocol_lock_enabled="));
  Serial.println((unsigned)protocol_lock.enabled);
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);
  Serial.println(F("planned_target_transport=uart"));
  Serial.print(F("planned_target_baud="));
  Serial.println((unsigned long)TARGET_UART_BAUD);

  if (APPLY_CHANGES == 0U) {
    return;
  }
  if (protocol_lock.enabled != 0U) {
    ezo_arduino_fail_fast("protocol_lock_enabled", EZO_ERR_PROTOCOL);
  }

  EZO_ARDUINO_CHECK_OK("switch_to_uart",
                       ezo_control_send_switch_to_uart_i2c(
                           &device, info.product_id, TARGET_UART_BAUD, &hint));
  ezo_arduino_wait_hint(&hint);

  Serial.println(F("post_transport=uart"));
  Serial.print(F("reconnect_uart_baud="));
  Serial.println((unsigned long)TARGET_UART_BAUD);
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_device",
                       ezo_arduino_i2c_init_device(&device, DEVICE_I2C_ADDRESS, &wire_context));
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
