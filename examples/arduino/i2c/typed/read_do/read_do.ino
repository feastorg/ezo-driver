/*
Purpose: simple Arduino I2C dissolved-oxygen read with an explicit output-config query.
Defaults: Wire on the board default pins and address 97.
Assumptions: the device is a D.O. circuit already in I2C mode.
Next: read ../../commissioning/inspect_device/inspect_device.ino for setup checks.
*/

#include <Wire.h>

#include <ezo_do.h>
#include <ezo_i2c_arduino_wire.h>

#include <string.h>

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const uint8_t DO_I2C_ADDRESS = 97U;
static const size_t DEBUG_RAW_BUFFER_LEN = 64U;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static ezo_do_output_config_t output_config;
static unsigned long startup_started_at_ms = 0;
static uint8_t read_requested = 0;

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

static void wait_hint(const ezo_timing_hint_t *hint) {
  delay(hint->wait_ms);
}

static void print_hex_u8(uint8_t value) {
  if (value < 16U) {
    Serial.print('0');
  }
  Serial.print((unsigned long)value, HEX);
}

static void print_hex_bytes(const uint8_t *data, size_t len) {
  size_t i = 0;
  for (i = 0; i < len; ++i) {
    if (i > 0U) {
      Serial.print(' ');
    }
    Serial.print("0x");
    print_hex_u8(data[i]);
  }
}

static void debug_output_query_response(const uint8_t *raw,
                                        size_t raw_len,
                                        ezo_device_status_t status) {
  char text[DEBUG_RAW_BUFFER_LEN + 1U];
  size_t copy_len = raw_len;

  if (copy_len > DEBUG_RAW_BUFFER_LEN) {
    copy_len = DEBUG_RAW_BUFFER_LEN;
  }
  memset(text, 0, sizeof(text));
  if (copy_len > 0U) {
    memcpy(text, raw, copy_len);
  }

  Serial.print("debug_output_query_status=");
  Serial.print(ezo_device_status_name(status));
  Serial.print(" raw_len=");
  Serial.print((unsigned long)raw_len);
  Serial.print(" raw_hex=");
  print_hex_bytes(raw, raw_len);
  Serial.println();

  Serial.print("debug_output_query_text=");
  Serial.println(text);
}

static void request_output_config() {
  ezo_timing_hint_t hint;
  uint8_t raw[DEBUG_RAW_BUFFER_LEN];
  size_t raw_len = 0U;
  size_t text_len = 0U;
  ezo_device_status_t status = EZO_STATUS_UNKNOWN;
  ezo_result_t rc = EZO_OK;

  rc = ezo_do_send_output_query_i2c(&device, &hint);
  CHECK_OK("send_output_query", rc);

  Serial.print("debug_output_query_wait_ms=");
  Serial.println((unsigned long)hint.wait_ms);
  wait_hint(&hint);

  rc = ezo_read_response_raw(&device, raw, sizeof(raw), &raw_len, &status);
  CHECK_OK("read_output_query_raw", rc);
  debug_output_query_response(raw, raw_len, status);

  for (text_len = 0U; text_len < raw_len; ++text_len) {
    if (raw[text_len] == 0U) {
      break;
    }
  }

  if (status != EZO_STATUS_SUCCESS) {
    Serial.print("debug_output_query_non_success_status=");
    Serial.println(ezo_device_status_name(status));
    fail_fast("read_output_query_status", EZO_ERR_PROTOCOL);
  }

  rc = ezo_do_parse_output_config((const char *)raw, text_len, &output_config);
  Serial.print("debug_output_query_parse_rc=");
  Serial.print((int)rc);
  Serial.print(" parse_name=");
  Serial.println(ezo_result_name(rc));
  CHECK_OK("parse_output_query", rc);

  Serial.print("debug_output_mask=");
  Serial.println((unsigned long)output_config.enabled_mask);
}

static void request_reading() {
  ezo_timing_hint_t hint;

  CHECK_OK("send_read", ezo_do_send_read_i2c(&device, &hint));
  wait_hint(&hint);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  CHECK_OK("init_wire_context", ezo_arduino_wire_context_init(&wire_context, &Wire));
  CHECK_OK("init_device",
           ezo_device_init(&device, DO_I2C_ADDRESS, ezo_arduino_wire_transport(), &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  ezo_do_reading_t reading;

  if (read_requested == 0U) {
    if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
      return;
    }

    request_output_config();
    request_reading();
    read_requested = 1U;
    return;
  }

  CHECK_OK("read_response", ezo_do_read_response_i2c(&device, output_config.enabled_mask, &reading));

  Serial.print("output_mask=");
  Serial.println((unsigned long)output_config.enabled_mask);
  if ((reading.present_mask & EZO_DO_OUTPUT_MG_L) != 0U) {
    Serial.print("milligrams_per_liter=");
    Serial.println(reading.milligrams_per_liter, 3);
  }
  if ((reading.present_mask & EZO_DO_OUTPUT_PERCENT_SATURATION) != 0U) {
    Serial.print("percent_saturation=");
    Serial.println(reading.percent_saturation, 3);
  }

  delay(1000);
  request_reading();
}
