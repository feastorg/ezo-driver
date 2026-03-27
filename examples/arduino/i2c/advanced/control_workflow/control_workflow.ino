/*
Purpose: inspect shared I2C control state and optionally apply name, LED, protocol-lock, find, sleep, or factory-reset operations.
Defaults: Wire on the board default pins and address 99.
Assumptions: the device is already in I2C mode and responds at `DEVICE_I2C_ADDRESS`.
Next: read ../transport_switch/transport_switch.ino for the software path that changes transport mode.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_control.h>

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 99U;
static const uint8_t APPLY_CHANGES = 0U;
static const char *PLANNED_NAME = "tank";
static const int8_t PLANNED_LED = -1;
static const int8_t PLANNED_PROTOCOL_LOCK = -1;
static const uint8_t SEND_FIND = 0U;
static const uint8_t SEND_SLEEP = 0U;
static const uint8_t SEND_FACTORY_RESET = 0U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t workflow_done = 0U;

static void query_info(ezo_device_info_t *info_out) {
  ezo_timing_hint_t hint;

  EZO_ARDUINO_CHECK_OK("send_info_query",
                       ezo_control_send_info_query_i2c(&device, EZO_PRODUCT_UNKNOWN, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_info_query", ezo_control_read_info_i2c(&device, info_out));
}

static void run_workflow() {
  ezo_device_info_t info;
  ezo_timing_hint_t hint;
  ezo_control_name_t name;
  ezo_control_status_t status;
  ezo_control_led_status_t led;
  ezo_control_protocol_lock_status_t protocol_lock;

  ezo_arduino_i2c_drain_pending(&device);
  query_info(&info);

  EZO_ARDUINO_CHECK_OK("send_name_query",
                       ezo_control_send_name_query_i2c(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_name_query", ezo_control_read_name_i2c(&device, &name));

  EZO_ARDUINO_CHECK_OK("send_status_query",
                       ezo_control_send_status_query_i2c(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_status_query", ezo_control_read_status_i2c(&device, &status));

  EZO_ARDUINO_CHECK_OK("send_led_query",
                       ezo_control_send_led_query_i2c(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_led_query", ezo_control_read_led_i2c(&device, &led));

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
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);
  Serial.print(F("planned_name="));
  Serial.println(PLANNED_NAME);
  Serial.print(F("planned_led="));
  Serial.println((int)PLANNED_LED);
  Serial.print(F("planned_protocol_lock="));
  Serial.println((int)PLANNED_PROTOCOL_LOCK);
  Serial.print(F("planned_send_find="));
  Serial.println((unsigned)SEND_FIND);
  Serial.print(F("planned_send_sleep="));
  Serial.println((unsigned)SEND_SLEEP);
  Serial.print(F("planned_factory_reset="));
  Serial.println((unsigned)SEND_FACTORY_RESET);

  if (APPLY_CHANGES == 0U) {
    return;
  }

  if (PLANNED_NAME[0] != '\0') {
    EZO_ARDUINO_CHECK_OK("send_name_set",
                         ezo_control_send_name_set_i2c(&device, info.product_id, PLANNED_NAME, &hint));
    ezo_arduino_wait_hint(&hint);
  }
  if (PLANNED_LED >= 0) {
    EZO_ARDUINO_CHECK_OK("send_led_set",
                         ezo_control_send_led_set_i2c(&device, info.product_id, (uint8_t)PLANNED_LED, &hint));
    ezo_arduino_wait_hint(&hint);
  }
  if (PLANNED_PROTOCOL_LOCK >= 0) {
    EZO_ARDUINO_CHECK_OK("send_protocol_lock_set",
                         ezo_control_send_protocol_lock_set_i2c(
                             &device, info.product_id, (uint8_t)PLANNED_PROTOCOL_LOCK, &hint));
    ezo_arduino_wait_hint(&hint);
  }
  if (SEND_FIND != 0U) {
    EZO_ARDUINO_CHECK_OK("send_find",
                         ezo_control_send_find_i2c(&device, info.product_id, &hint));
    ezo_arduino_wait_hint(&hint);
  }
  if (SEND_SLEEP != 0U) {
    EZO_ARDUINO_CHECK_OK("send_sleep",
                         ezo_control_send_sleep_i2c(&device, info.product_id, &hint));
    ezo_arduino_wait_hint(&hint);
  }
  if (SEND_FACTORY_RESET != 0U) {
    EZO_ARDUINO_CHECK_OK("send_factory_reset",
                         ezo_control_send_factory_reset_i2c(&device, info.product_id, &hint));
    ezo_arduino_wait_hint(&hint);
  }

  if (SEND_SLEEP != 0U || SEND_FACTORY_RESET != 0U) {
    Serial.println(F("post_query_skipped=1"));
    return;
  }

  EZO_ARDUINO_CHECK_OK("send_name_query_post",
                       ezo_control_send_name_query_i2c(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_name_query_post", ezo_control_read_name_i2c(&device, &name));
  EZO_ARDUINO_CHECK_OK("send_led_query_post",
                       ezo_control_send_led_query_i2c(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_led_query_post", ezo_control_read_led_i2c(&device, &led));
  EZO_ARDUINO_CHECK_OK("send_protocol_lock_query_post",
                       ezo_control_send_protocol_lock_query_i2c(&device, info.product_id, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_protocol_lock_query_post",
                       ezo_control_read_protocol_lock_i2c(&device, &protocol_lock));

  Serial.print(F("post_name="));
  Serial.println(name.name);
  Serial.print(F("post_led_enabled="));
  Serial.println((unsigned)led.enabled);
  Serial.print(F("post_protocol_lock_enabled="));
  Serial.println((unsigned)protocol_lock.enabled);
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_device",
                       ezo_arduino_i2c_init_device(&device, DEVICE_I2C_ADDRESS, &wire_context));
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
