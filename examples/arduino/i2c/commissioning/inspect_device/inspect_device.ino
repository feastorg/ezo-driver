/*
Purpose: inspect an Arduino I2C device and print basic identity/status information.
Defaults: Wire on the board default pins and `DEVICE_I2C_ADDRESS`.
Assumptions: the device is already in I2C mode and wired to the selected bus.
Next: read ../../typed/read_ph/read_ph.ino for the smallest typed read path.
*/

#include <Wire.h>

#include <ezo_control.h>
#include <ezo_i2c_arduino_wire.h>
#include <ezo_product.h>

enum {
  STARTUP_SETTLE_MS = 1000U,
  DEVICE_I2C_ADDRESS = 99U
};

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t inspection_done = 0;

static void fail_fast(const char *step, ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

  Serial.print("driver_error_step=");
  Serial.println(step);
  Serial.print("driver_error_name=");
  Serial.println(ezo_result_name(result));
  Serial.print("driver_error_code=");
  Serial.println((int)result);
  while (true) {
  }
}

#define CHECK_OK(step, expr) fail_fast(step, (expr))

static void inspect_once() {
  ezo_timing_hint_t hint;
  ezo_device_info_t info;
  ezo_control_status_t status;
  const ezo_product_metadata_t *metadata = NULL;

  CHECK_OK("send_info_query", ezo_control_send_info_query_i2c(&device, EZO_PRODUCT_UNKNOWN, &hint));
  delay(hint.wait_ms);
  CHECK_OK("read_info_query", ezo_control_read_info_i2c(&device, &info));

  CHECK_OK("send_status_query", ezo_control_send_status_query_i2c(&device, info.product_id, &hint));
  delay(hint.wait_ms);
  CHECK_OK("read_status_query", ezo_control_read_status_i2c(&device, &status));

  metadata = ezo_product_get_metadata(info.product_id);

  Serial.print("product_code=");
  Serial.println(info.product_code);
  Serial.print("firmware_version=");
  Serial.println(info.firmware_version);
  Serial.print("restart_code=");
  Serial.println(status.restart_code);
  Serial.print("supply_voltage_v=");
  Serial.println(status.supply_voltage, 3);
  if (metadata != NULL) {
    Serial.print("default_i2c_address=");
    Serial.println(metadata->default_i2c_address);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.print("configured_i2c_address=");
  Serial.println(DEVICE_I2C_ADDRESS);

  CHECK_OK("init_wire_context", ezo_arduino_wire_context_init(&wire_context, &Wire));
  CHECK_OK(
      "init_device",
      ezo_device_init(&device, DEVICE_I2C_ADDRESS, ezo_arduino_wire_transport(), &wire_context));
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
