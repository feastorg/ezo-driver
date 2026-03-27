/*
Purpose: inspect calibration export chunks and optionally apply one import payload over Arduino I2C.
Defaults: Wire on the board default pins, address 99, and product pH.
Assumptions: the selected product supports calibration transfer and import payloads are trusted.
Next: read ../../commissioning/inspect_device/inspect_device.ino before moving calibration between devices.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_calibration_transfer.h>

#include <string.h>

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 99U;
static const ezo_product_id_t PRODUCT_ID = EZO_PRODUCT_PH;
static const uint8_t APPLY_IMPORT = 0U;
static const char *IMPORT_PAYLOAD = "";
static const size_t EXPORT_CHUNK_CAPACITY = 128U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t transfer_done = 0U;

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_calibration_export_info_t export_info;
  ezo_calibration_import_result_t import_result;
  char chunk[EXPORT_CHUNK_CAPACITY];
  size_t chunk_len = 0U;
  uint32_t chunk_index = 0U;
  const ezo_product_metadata_t *metadata = ezo_arduino_product_metadata(PRODUCT_ID);

  if (metadata == NULL ||
      !ezo_product_supports_capability(PRODUCT_ID, EZO_PRODUCT_CAP_CALIBRATION_TRANSFER)) {
    ezo_arduino_fail_fast("unsupported_calibration_transfer", EZO_ERR_INVALID_ARGUMENT);
  }

  EZO_ARDUINO_CHECK_OK("send_export_info_query",
                       ezo_calibration_send_export_info_query_i2c(&device, PRODUCT_ID, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_export_info",
                       ezo_calibration_read_export_info_i2c(&device, &export_info));

  Serial.println(F("transport=i2c"));
  Serial.print(F("product="));
  Serial.println(metadata->vendor_short_code);
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("export_chunk_count="));
  Serial.println((unsigned long)export_info.chunk_count);
  Serial.print(F("export_byte_count="));
  Serial.println((unsigned long)export_info.byte_count);

  for (chunk_index = 0U; chunk_index < export_info.chunk_count; ++chunk_index) {
    EZO_ARDUINO_CHECK_OK("send_export_next",
                         ezo_calibration_send_export_next_i2c(&device, PRODUCT_ID, &hint));
    ezo_arduino_wait_hint(&hint);
    EZO_ARDUINO_CHECK_OK("read_export_chunk",
                         ezo_calibration_read_export_chunk_i2c(
                             &device, chunk, sizeof(chunk), &chunk_len));
    Serial.print(F("export_chunk_"));
    Serial.print((unsigned long)chunk_index);
    Serial.print(F("="));
    Serial.write((const uint8_t *)chunk, chunk_len);
    Serial.println();
  }

  Serial.print(F("apply_import="));
  Serial.println((unsigned)APPLY_IMPORT);
  Serial.print(F("import_payload_length="));
  Serial.println((unsigned long)strlen(IMPORT_PAYLOAD));

  if (APPLY_IMPORT == 0U) {
    return;
  }

  EZO_ARDUINO_CHECK_OK("send_import",
                       ezo_calibration_send_import_i2c(&device, PRODUCT_ID, IMPORT_PAYLOAD, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_import_result",
                       ezo_calibration_read_import_result_i2c(&device, &import_result));

  Serial.print(F("import_device_status="));
  Serial.println(ezo_device_status_name(import_result.device_status));
  Serial.print(F("import_pending_reboot="));
  Serial.println((unsigned)import_result.pending_reboot);
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_device",
                       ezo_arduino_i2c_init_device(&device, DEVICE_I2C_ADDRESS, &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  if (transfer_done != 0U) {
    return;
  }

  if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
    return;
  }

  run_workflow();
  transfer_done = 1U;
}
