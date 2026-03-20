# API Contract

Status: accepted

Source of truth:

- public C API: `src/ezo_i2c.h`
- public C++ API: `src/ezo_i2c.hpp`

This document only records the contract decisions that matter at the repo level. It does not duplicate every declaration from the headers.

## Core Rules

1. The core is C99.
2. The core is synchronous.
3. The core is transport-agnostic.
4. The core does not allocate dynamically.
5. The core does not sleep internally.
6. Callers own timing and buffers.
7. Library results and device status are separate.

## Public Surface

The v1 public surface provides:

- device init and address accessors
- timing hints by command kind
- generic command send helpers
- read helpers for plain read and read-with-temperature-compensation
- text response reads
- raw response reads
- decimal parsing helper
- thin C++ wrapper over the same C surface

Explicitly out of scope for v1:

- typed pH/EC/RTD helper APIs
- async/state-machine behavior
- hidden retries or hidden delays
- compatibility with the legacy Atlas API shape

Primary C entry points:

- `ezo_device_init()`
- `ezo_send_command()`
- `ezo_send_command_with_float()`
- `ezo_send_read()`
- `ezo_send_read_with_temp_comp()`
- `ezo_read_response()`
- `ezo_read_response_raw()`
- `ezo_parse_double()`

## Transport Contract

The core depends on one injected transport callback:

- `write_then_read(context, address, tx_data, tx_len, rx_data, rx_len, rx_received)`

Transport responsibilities:

- perform the bus transaction
- report transport success or failure
- report the actual read byte count

Core responsibilities:

- format commands
- request and decode responses
- map status bytes
- parse payload text
- return timing hints

## Status And Errors

Known device status-byte mapping:

- `1` -> `EZO_STATUS_SUCCESS`
- `2` -> `EZO_STATUS_FAIL`
- `254` -> `EZO_STATUS_NOT_READY`
- `255` -> `EZO_STATUS_NO_DATA`

Rules:

1. Transport failures return `EZO_ERR_TRANSPORT` and leave device status as `EZO_STATUS_UNKNOWN`.
2. Unknown status bytes return `EZO_ERR_PROTOCOL` and leave device status as `EZO_STATUS_UNKNOWN`.
3. Valid but unsuccessful device responses still return `EZO_OK`; callers inspect device status.
4. Parse failures return `EZO_ERR_PARSE` only after a successful device response.

## Response Semantics

`ezo_read_response()` is the text path.

Rules:

1. The buffer is null-terminated on success.
2. `response_len` excludes the terminating null.
3. The caller must provide room for payload plus terminating null.
4. If the payload fits exactly with no room for the null terminator, the result is `EZO_ERR_BUFFER_TOO_SMALL`.
5. Text buffers are limited by `EZO_I2C_MAX_TEXT_RESPONSE_LEN`.

`ezo_read_response_raw()` is the byte-preserving path.

Rules:

1. The payload is copied exactly as returned after the status byte.
2. No null termination is added.
3. Embedded zero bytes are preserved.
4. Raw buffers are limited by `EZO_I2C_MAX_RESPONSE_PAYLOAD_LEN`.
5. Oversized payloads return `EZO_ERR_BUFFER_TOO_SMALL`.

Callers choose the text or raw path explicitly. The library does not infer payload type.

## Timing Semantics

The core never sleeps.

Default command-class hints in v1:

- generic command: `300 ms`
- read: `1000 ms`
- read with temperature compensation: `1000 ms`
- calibration: `1200 ms`

These are conservative defaults derived from `_reference/`. They are hints, not guarantees.

## Numeric Semantics

`ezo_send_command_with_float()` uses the library's own fixed-point formatter.

`ezo_parse_double()` accepts the subset of decimal text needed for EZO payloads:

- optional leading or trailing ASCII whitespace
- optional sign
- optional fractional part
- no exponent syntax

## Call Pattern

Typical command flow:

1. initialize a device with a transport callback and context
2. send a command and read the timing hint
3. wait outside the library
4. read either a text or raw response
5. inspect device status separately from the library result
