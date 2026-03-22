/*
Purpose: inspect an Arduino I2C device and print basic identity/status information.
Defaults: Wire on the board default pins and address 99.
Assumptions: the device is already in I2C mode and wired to the selected bus.
Next: read ../../typed/read_ph/read_ph.ino for the smallest typed read path.
*/

#include <Wire.h>

#include <ezo_control.h>
#include <ezo_i2c_arduino_wire.h>
#include <ezo_product.h>

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;

static void fail_fast(ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

  Serial.print("driver_error=");
  Serial.println((int)result);
  while (true) {
  }
}

void setup() {
  ezo_timing_hint_t hint;
  ezo_device_info_t info;
  ezo_control_status_t status;
  const ezo_product_metadata_t *metadata = NULL;

  Serial.begin(115200);
  Wire.begin();

  fail_fast(ezo_arduino_wire_context_init(&wire_context, &Wire));
  fail_fast(ezo_device_init(&device, 99, ezo_arduino_wire_transport(), &wire_context));

  fail_fast(ezo_control_send_info_query_i2c(&device, EZO_PRODUCT_UNKNOWN, &hint));
  delay(hint.wait_ms);
  fail_fast(ezo_control_read_info_i2c(&device, &info));

  fail_fast(ezo_control_send_status_query_i2c(&device, info.product_id, &hint));
  delay(hint.wait_ms);
  fail_fast(ezo_control_read_status_i2c(&device, &status));

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

void loop() {
}
