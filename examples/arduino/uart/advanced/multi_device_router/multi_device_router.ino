/*
Purpose: route one Arduino UART transport across multiple EZO channels and poll them explicitly.
Defaults: AVR boards use SoftwareSerial on pins 10/11 and route-select pins 4/5/6; ESP32 uses Serial1 on pins 16/17 and route-select pins 18/19/21.
Assumptions: this is a hardware-topology-specific sketch for a 3-bit UART channel selector, routed modules are in discrete-response mode, and response-code mode may be bootstrapped.
Next: read ../../commissioning/inspect_device/inspect_device.ino for the single-device bootstrap path and the Linux advanced compensation examples for cross-device workflows.
*/

#include <Arduino.h>

#include <ezo_control.h>
#include <ezo_uart.h>
#include <ezo_uart_arduino_stream.h>

typedef struct routed_module_t routed_module_t;

#if defined(ARDUINO_ARCH_AVR)
#include <SoftwareSerial.h>
static const uint8_t SENSOR_RX_PIN = 10U;
static const uint8_t SENSOR_TX_PIN = 11U;
static const uint8_t ROUTE_SELECT_PIN_0 = 4U;
static const uint8_t ROUTE_SELECT_PIN_1 = 5U;
static const uint8_t ROUTE_SELECT_PIN_2 = 6U;
static SoftwareSerial sensor_serial(SENSOR_RX_PIN, SENSOR_TX_PIN);
static Stream *sensor_stream() {
  return &sensor_serial;
}
static void begin_streams() {
  Serial.begin(115200);
  sensor_serial.begin(9600);
}
#elif defined(ARDUINO_ARCH_ESP32)
static const int SENSOR_RX_PIN = 16;
static const int SENSOR_TX_PIN = 17;
static const uint8_t ROUTE_SELECT_PIN_0 = 18U;
static const uint8_t ROUTE_SELECT_PIN_1 = 19U;
static const uint8_t ROUTE_SELECT_PIN_2 = 21U;
static Stream *sensor_stream() {
  return &Serial1;
}
static void begin_streams() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, SENSOR_RX_PIN, SENSOR_TX_PIN);
}
#elif defined(HAVE_HWSERIAL1)
static const uint8_t ROUTE_SELECT_PIN_0 = 4U;
static const uint8_t ROUTE_SELECT_PIN_1 = 5U;
static const uint8_t ROUTE_SELECT_PIN_2 = 6U;
static Stream *sensor_stream() {
  return &Serial1;
}
static void begin_streams() {
  Serial.begin(115200);
  Serial1.begin(9600);
}
#else
#error "This sketch requires either SoftwareSerial or Serial1 for the routed sensor bus."
#endif

struct routed_module_t {
  uint8_t channel;
  const char *label;
  ezo_product_id_t product_id;
};

static const unsigned long STARTUP_SETTLE_MS = 1000UL;
static const unsigned long POLL_INTERVAL_MS = 1200UL;
static const routed_module_t MODULES[] = {
    {1U, "PH", EZO_PRODUCT_PH},
    {2U, "EC", EZO_PRODUCT_EC},
    {3U, "DO", EZO_PRODUCT_DO},
};

enum {
  MODULE_COUNT = sizeof(MODULES) / sizeof(MODULES[0]),
  ROUTED_LINE_BUFFER_LEN = EZO_UART_MAX_TEXT_RESPONSE_CAPACITY
};

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;
static unsigned long startup_started_at_ms = 0;
static unsigned long last_poll_started_at_ms = 0;
static uint8_t bootstrap_done = 0U;
static uint8_t current_module_index = 0U;

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

static void select_channel(uint8_t channel) {
  uint8_t channel_bits = 0;

  if (channel < 1U) {
    channel = 1U;
  }

  channel_bits = (uint8_t)(channel - 1U);
  digitalWrite(ROUTE_SELECT_PIN_0, bitRead(channel_bits, 0));
  digitalWrite(ROUTE_SELECT_PIN_1, bitRead(channel_bits, 1));
  digitalWrite(ROUTE_SELECT_PIN_2, bitRead(channel_bits, 2));
  delay(2);
}

static void ensure_response_codes_enabled(ezo_product_id_t product_id) {
  ezo_timing_hint_t hint;
  ezo_control_response_code_status_t response_code;

  CHECK_OK("discard_input_before_bootstrap", ezo_uart_discard_input(&device));
  CHECK_OK("send_response_code_query",
           ezo_control_send_response_code_query_uart(&device, product_id, &hint));
  wait_hint(&hint);
  CHECK_OK("read_response_code_query", ezo_control_read_response_code_uart(&device, &response_code));

  if (response_code.enabled != 0U) {
    return;
  }

  CHECK_OK("send_response_code_set",
           ezo_control_send_response_code_set_uart(&device, product_id, 1, &hint));
  wait_hint(&hint);
  CHECK_OK("read_response_code_ack", ezo_uart_read_ok(&device));
}

static void print_identity(const routed_module_t *module) {
  ezo_timing_hint_t hint;
  ezo_device_info_t info;

  select_channel(module->channel);
  ensure_response_codes_enabled(module->product_id);
  CHECK_OK("send_info_query",
           ezo_control_send_info_query_uart(&device, module->product_id, &hint));
  wait_hint(&hint);
  CHECK_OK("read_info_query", ezo_control_read_info_uart(&device, &info));

  Serial.print("boot_channel=");
  Serial.println(module->channel);
  Serial.print("boot_label=");
  Serial.println(module->label);
  Serial.print("boot_product_code=");
  Serial.println(info.product_code);
  Serial.print("boot_firmware_version=");
  Serial.println(info.firmware_version);
}

static void bootstrap_modules() {
  uint8_t index = 0;

  for (index = 0U; index < MODULE_COUNT; ++index) {
    print_identity(&MODULES[index]);
  }

  bootstrap_done = 1U;
}

static void poll_module(const routed_module_t *module) {
  ezo_timing_hint_t hint;
  ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN;
  char buffer[ROUTED_LINE_BUFFER_LEN];
  size_t response_len = 0U;

  select_channel(module->channel);
  CHECK_OK("discard_input_before_read", ezo_uart_discard_input(&device));
  CHECK_OK("send_read", ezo_uart_send_read(&device, &hint));
  wait_hint(&hint);
  CHECK_OK("read_line",
           ezo_uart_read_line(&device, buffer, sizeof(buffer), &response_len, &kind));
  if (kind != EZO_UART_RESPONSE_DATA) {
    fail_fast("expected_data_line", EZO_ERR_PROTOCOL);
  }
  CHECK_OK("read_ok", ezo_uart_read_ok(&device));

  Serial.print("channel=");
  Serial.println(module->channel);
  Serial.print("label=");
  Serial.println(module->label);
  Serial.print("raw_reading=");
  Serial.println(buffer);
  Serial.println();
}

void setup() {
  begin_streams();

  pinMode(ROUTE_SELECT_PIN_0, OUTPUT);
  pinMode(ROUTE_SELECT_PIN_1, OUTPUT);
  pinMode(ROUTE_SELECT_PIN_2, OUTPUT);
  select_channel(1U);

  CHECK_OK("init_uart_context", ezo_uart_arduino_stream_context_init(&uart_context, sensor_stream()));
  CHECK_OK("init_uart_device",
           ezo_uart_device_init(&device, ezo_uart_arduino_stream_transport(), &uart_context));
  startup_started_at_ms = millis();
}

void loop() {
  if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
    return;
  }

  if (bootstrap_done == 0U) {
    bootstrap_modules();
    last_poll_started_at_ms = millis();
    return;
  }

  if ((unsigned long)(millis() - last_poll_started_at_ms) < POLL_INTERVAL_MS) {
    return;
  }

  last_poll_started_at_ms = millis();
  poll_module(&MODULES[current_module_index]);
  current_module_index = (uint8_t)((current_module_index + 1U) % MODULE_COUNT);
}
