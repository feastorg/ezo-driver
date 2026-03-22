/*
Purpose: minimal Arduino UART raw smoke path.
Defaults: 9600 baud on Serial1 when available, otherwise Serial.
Assumptions: the sensor UART is wired to the chosen stream.
Next: read ../../commissioning/inspect_device/inspect_device.ino for bootstrap and identity checks.
*/

// Minimal UART smoke path.
// On boards without a second hardware serial port, this example uses `Serial` as the
// sensor stream and does not emit debug output.

#include <ezo_uart.h>
#include <ezo_uart_arduino_stream.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(HAVE_HWSERIAL1)
#define EZO_UART_HAS_DEBUG_STREAM 1
#else
#define EZO_UART_HAS_DEBUG_STREAM 0
#endif

enum {
  STARTUP_SETTLE_MS = 1000U
};

static ezo_uart_arduino_stream_context_t uart_context;
static ezo_uart_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t smoke_sent = 0;

static Stream *sensor_stream() {
#if EZO_UART_HAS_DEBUG_STREAM
  return &Serial1;
#else
  return &Serial;
#endif
}

static void begin_sensor_stream() {
#if EZO_UART_HAS_DEBUG_STREAM
  Serial.begin(115200);
  Serial1.begin(9600);
#else
  Serial.begin(9600);
#endif
}

static void fail_fast(const char *step, ezo_result_t result) {
  if (result == EZO_OK) {
    return;
  }

#if EZO_UART_HAS_DEBUG_STREAM
  Serial.print("driver_error_step=");
  Serial.println(step);
  Serial.print("driver_error_name=");
  Serial.println(ezo_result_name(result));
  Serial.print("driver_error_code=");
  Serial.println((int)result);
#endif

  while (true) {
  }
}

#define CHECK_OK(step, expr) fail_fast(step, (expr))

static void send_smoke_command() {
  ezo_timing_hint_t hint;
  CHECK_OK("send_smoke_command", ezo_uart_send_command(&device, "i", EZO_COMMAND_GENERIC, &hint));

#if EZO_UART_HAS_DEBUG_STREAM
  Serial.println("smoke_sent=1");
#endif
}

void setup() {
  begin_sensor_stream();
  CHECK_OK("init_uart_context", ezo_uart_arduino_stream_context_init(&uart_context, sensor_stream()));
  CHECK_OK("init_uart_device",
           ezo_uart_device_init(&device, ezo_uart_arduino_stream_transport(), &uart_context));
  startup_started_at_ms = millis();
}

void loop() {
  if (smoke_sent != 0U) {
    return;
  }

  if ((unsigned long)(millis() - startup_started_at_ms) < STARTUP_SETTLE_MS) {
    return;
  }

  send_smoke_command();
  smoke_sent = 1U;
}
