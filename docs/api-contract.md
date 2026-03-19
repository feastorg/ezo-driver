# API Contract

Status: Accepted for implementation

## Scope

This document defines the implementation-facing contract for the v1 C core.

It covers:

- public C API shape
- transport contract
- error and status semantics
- timing semantics
- canonical call flow

It does not define:

- typed device helpers
- async behavior
- Arduino- or Linux-specific adapter APIs

## Core Rules

1. The core is C99.
2. The core is synchronous.
3. The core does not allocate dynamically.
4. The core does not sleep internally.
5. The core does not depend on Arduino or Linux SDK types.
6. Library errors and device status remain separate.
7. The core uses fixed internal buffers, not VLAs or heap allocation.

## Public Types

```c
#define EZO_I2C_MAX_TEXT_RESPONSE_LEN 255

typedef enum {
  EZO_OK = 0,
  EZO_ERR_INVALID_ARGUMENT,
  EZO_ERR_BUFFER_TOO_SMALL,
  EZO_ERR_TRANSPORT,
  EZO_ERR_PROTOCOL,
  EZO_ERR_PARSE,
  EZO_ERR_STATE
} ezo_result_t;

typedef enum {
  EZO_STATUS_UNKNOWN = 0,
  EZO_STATUS_SUCCESS,
  EZO_STATUS_FAIL,
  EZO_STATUS_NOT_READY,
  EZO_STATUS_NO_DATA
} ezo_device_status_t;

typedef enum {
  EZO_COMMAND_GENERIC = 0,
  EZO_COMMAND_READ,
  EZO_COMMAND_READ_WITH_TEMP_COMP,
  EZO_COMMAND_CALIBRATION
} ezo_command_kind_t;

typedef struct {
  uint32_t wait_ms;
} ezo_timing_hint_t;

typedef struct ezo_i2c_transport {
  ezo_result_t (*write_then_read)(void *context,
                                  uint8_t address,
                                  const uint8_t *tx_data,
                                  size_t tx_len,
                                  uint8_t *rx_data,
                                  size_t rx_len,
                                  size_t *rx_received);
} ezo_i2c_transport_t;

typedef struct {
  uint8_t address;
  const ezo_i2c_transport_t *transport;
  void *transport_context;
  uint8_t last_device_status;
} ezo_i2c_device_t;
```

## Public API Surface

```c
ezo_result_t ezo_device_init(ezo_i2c_device_t *device,
                             uint8_t address,
                             const ezo_i2c_transport_t *transport,
                             void *transport_context);

void ezo_device_set_address(ezo_i2c_device_t *device, uint8_t address);
uint8_t ezo_device_get_address(const ezo_i2c_device_t *device);
ezo_device_status_t ezo_device_get_last_status(const ezo_i2c_device_t *device);

ezo_result_t ezo_get_timing_hint_for_command_kind(ezo_command_kind_t kind,
                                                  ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_send_command(ezo_i2c_device_t *device,
                              const char *command,
                              ezo_command_kind_t kind,
                              ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_send_command_with_float(ezo_i2c_device_t *device,
                                         const char *prefix,
                                         double value,
                                         uint8_t decimals,
                                         ezo_command_kind_t kind,
                                         ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_send_read(ezo_i2c_device_t *device,
                           ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_send_read_with_temp_comp(ezo_i2c_device_t *device,
                                          double temperature_c,
                                          uint8_t decimals,
                                          ezo_timing_hint_t *timing_hint);

ezo_result_t ezo_read_response(ezo_i2c_device_t *device,
                               char *buffer,
                               size_t buffer_len,
                               size_t *response_len,
                               ezo_device_status_t *device_status);

ezo_result_t ezo_parse_double(const char *buffer,
                              size_t buffer_len,
                              double *value_out);
```

## Transport Contract

The transport boundary uses one transaction primitive in v1.

Semantics:

- send-only: non-null `tx_data`, zero-length read side
- read-only: zero-length write side, non-null `rx_data`
- `rx_received` reports the actual byte count

Transport responsibilities:

- perform the bus transaction
- return transport success/failure
- report read byte count

Core responsibilities:

- format commands
- request responses
- decode device status bytes
- parse payloads
- expose timing hints

## Error And Status Semantics

Library result and device status are separate.

Reference status-byte mapping:

- `1 -> EZO_STATUS_SUCCESS`
- `2 -> EZO_STATUS_FAIL`
- `254 -> EZO_STATUS_NOT_READY`
- `255 -> EZO_STATUS_NO_DATA`

Rules:

1. Transport failure:
   - result: `EZO_ERR_TRANSPORT`
   - status: `EZO_STATUS_UNKNOWN`

2. Unknown status byte:
   - result: `EZO_ERR_PROTOCOL`
   - status: `EZO_STATUS_UNKNOWN`

3. Valid but unsuccessful device response:
   - result: `EZO_OK`
   - status: one of `FAIL`, `NOT_READY`, `NO_DATA`

4. Successful response with parse failure:
   - result: `EZO_ERR_PARSE`
   - status: `EZO_STATUS_SUCCESS`

## Buffer Semantics

`ezo_read_response()` is text-oriented in v1.

Rules:

1. On success, the response buffer is null-terminated.
2. `response_len` reports the payload length excluding the terminating null.
3. The caller must provide enough space for payload plus terminating null.
4. If the payload would exactly fill the buffer with no room for the null terminator, the result is `EZO_ERR_BUFFER_TOO_SMALL`.
5. `buffer_len` must be at most `EZO_I2C_MAX_TEXT_RESPONSE_LEN`.

## Timing Semantics

The core never sleeps.

The caller is responsible for waiting according to the returned timing hint.

v1 default timing classes:

- generic command: `300 ms`
- read: `1000 ms`
- read with temperature compensation: `1000 ms`
- calibration: `1200 ms`

These are conservative class-level defaults derived from `_reference/`, not per-device guarantees.

## Numeric Helper Semantics

`ezo_send_command_with_float()` formats fixed-point decimal text without relying on libc floating-point formatting.

`ezo_parse_double()` parses plain decimal text intended for EZO payloads:

- optional leading/trailing ASCII whitespace
- optional sign
- optional fractional component
- no exponent syntax

`ezo_send_command_with_float()` accepts up to 9 decimal places in v1.

## Canonical Call Flow

### Generic query

```c
ezo_timing_hint_t hint;
ezo_device_status_t status;
char response[32];
size_t response_len = 0;

ezo_send_command(&device, "name,?", EZO_COMMAND_GENERIC, &hint);
/* caller waits hint.wait_ms */
ezo_read_response(&device, response, sizeof(response), &response_len, &status);
```

### Numeric read

```c
double value = 0.0;

ezo_send_read(&device, &hint);
/* caller waits hint.wait_ms */
ezo_read_response(&device, response, sizeof(response), &response_len, &status);

if (status == EZO_STATUS_SUCCESS) {
  ezo_parse_double(response, response_len, &value);
}
```

## Explicit Non-Goals

- preserve legacy `issued_read` / `NOT_READ_CMD` behavior
- preserve the old Arduino API shape
- add typed helpers in v1
- hide waiting or retries inside the core

## One Deferred Question

The only implementation-adjacent open question is whether a raw-byte response variant should be added later alongside the current text-oriented `ezo_read_response()` path.
