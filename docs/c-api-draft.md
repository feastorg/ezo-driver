# C API Draft

Status: Accepted for header scaffolding
Phase: 01

## Purpose

Define the first public C API for the portable EZO I2C core.

This API is intentionally:

- generic rather than device-specific
- synchronous
- transport-agnostic
- explicit about buffers, timing, and error handling

## Design Constraints

- C99 core
- no dynamic allocation in the core
- caller-owned buffers
- no Arduino or Linux types in public core headers
- library errors separated from device status codes

## Core Types

### Opaque transport handle

The device binds to a transport contract plus caller-owned transport context.

```c
typedef struct ezo_i2c_transport ezo_i2c_transport_t;
```

### Device

The device object stores only core interaction state.

```c
typedef struct {
  uint8_t address;
  const ezo_i2c_transport_t *transport;
  void *transport_context;
  uint8_t last_device_status;
} ezo_i2c_device_t;
```

### Result

Library-level result:

```c
typedef enum {
  EZO_OK = 0,
  EZO_ERR_INVALID_ARGUMENT,
  EZO_ERR_BUFFER_TOO_SMALL,
  EZO_ERR_TRANSPORT,
  EZO_ERR_PROTOCOL,
  EZO_ERR_PARSE,
  EZO_ERR_STATE
} ezo_result_t;
```

### Device status

Device-reported response status:

```c
typedef enum {
  EZO_STATUS_UNKNOWN = 0,
  EZO_STATUS_SUCCESS,
  EZO_STATUS_FAIL,
  EZO_STATUS_NOT_READY,
  EZO_STATUS_NO_DATA
} ezo_device_status_t;
```

### Command kind

Used for timing classification.

```c
typedef enum {
  EZO_COMMAND_GENERIC = 0,
  EZO_COMMAND_READ,
  EZO_COMMAND_READ_WITH_TEMP_COMP,
  EZO_COMMAND_CALIBRATION
} ezo_command_kind_t;
```

### Timing hint

```c
typedef struct {
  uint32_t wait_ms;
} ezo_timing_hint_t;
```

## Primary API Surface

### Device lifecycle

```c
ezo_result_t ezo_device_init(
    ezo_i2c_device_t *device,
    uint8_t address,
    const ezo_i2c_transport_t *transport,
    void *transport_context);

void ezo_device_set_address(ezo_i2c_device_t *device, uint8_t address);
uint8_t ezo_device_get_address(const ezo_i2c_device_t *device);
ezo_device_status_t ezo_device_get_last_status(const ezo_i2c_device_t *device);
```

### Raw command path

```c
ezo_result_t ezo_send_command(
    ezo_i2c_device_t *device,
    const char *command,
    ezo_command_kind_t kind,
    ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_read_response(
    ezo_i2c_device_t *device,
    char *buffer,
    size_t buffer_len,
    size_t *response_len,
    ezo_device_status_t *device_status);
```

### Numeric command helpers

```c
ezo_result_t ezo_send_command_with_float(
    ezo_i2c_device_t *device,
    const char *prefix,
    double value,
    uint8_t decimals,
    ezo_command_kind_t kind,
    ezo_timing_hint_t *timing_hint);
```

### Common generic convenience wrappers

These are still generic protocol helpers, not typed device APIs.

```c
ezo_result_t ezo_send_read(
    ezo_i2c_device_t *device,
    ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_send_read_with_temp_comp(
    ezo_i2c_device_t *device,
    double temperature_c,
    uint8_t decimals,
    ezo_timing_hint_t *timing_hint);
```

### Parse helpers

```c
ezo_result_t ezo_parse_double(
    const char *buffer,
    size_t buffer_len,
    double *value_out);
```

Future helpers may include split-field parsing, but only once real payload categories justify them.

### Timing helpers

```c
ezo_result_t ezo_get_timing_hint_for_command_kind(
    ezo_command_kind_t kind,
    ezo_timing_hint_t *timing_hint);
```

## Canonical Usage Pattern

### Generic command/response

```c
ezo_i2c_device_t device;
ezo_timing_hint_t hint;
char response[32];
size_t response_len;
ezo_device_status_t status;

ezo_device_init(&device, 0x63, transport, context);
ezo_send_command(&device, "name,?", EZO_COMMAND_GENERIC, &hint);
/* caller waits hint.wait_ms */
ezo_read_response(&device, response, sizeof(response), &response_len, &status);
```

### Read and parse

```c
double reading;

ezo_send_read(&device, &hint);
/* caller waits hint.wait_ms */
ezo_read_response(&device, response, sizeof(response), &response_len, &status);

if (status == EZO_STATUS_SUCCESS) {
  ezo_parse_double(response, response_len, &reading);
}
```

## Explicit Non-Goals In This API

- typed device helpers like `ezo_ph_read()`
- async API
- hidden sleeps
- internal heap allocation
- public dependency on `TwoWire`, `i2c-dev`, or C++ types

## Open Points To Confirm During Header Drafting

1. Whether `ezo_read_response()` should accept an optional raw byte buffer variant in addition to the text buffer path.
