# API Contract

Status: accepted

Source of truth:

- shared C API: `src/ezo.h`
- I2C C API: `src/ezo_i2c.h`
- I2C C++ API: `src/ezo_i2c.hpp`
- I2C Arduino adapter API: `src/ezo_i2c_arduino_wire.h`
- UART C API: `src/ezo_uart.h`
- UART Arduino adapter API: `src/ezo_uart_arduino_stream.h`

This document records repo-level contract decisions. It does not duplicate every declaration from the headers.

## Core Rules

1. The core is C99.
2. The core is synchronous.
3. The core does not allocate dynamically.
4. The core does not sleep internally.
5. Callers own timing and buffers.
6. Library results and device response state are separate.
7. I2C and UART use separate transport contracts and separate device types.

## Shared Public Surface

The shared public surface provides:

- `ezo_result_t`
- `ezo_command_kind_t`
- `ezo_timing_hint_t`
- `ezo_get_timing_hint_for_command_kind()`
- `ezo_parse_double()`

Shared timing hints remain:

- generic command: `300 ms`
- read: `1000 ms`
- read with temperature compensation: `1000 ms`
- calibration: `1200 ms`

`ezo_parse_double()` accepts the decimal subset needed for EZO payloads:

- optional leading or trailing ASCII whitespace
- optional sign
- optional fractional part
- no exponent syntax

## I2C Surface

The I2C API provides:

- device init and address accessors
- generic command send helpers
- read helpers for plain read and read-with-temperature-compensation
- text response reads
- raw response reads
- thin C++ wrapper over the same C surface
- Arduino `TwoWire` adapter surface

Primary I2C C entry points:

- `ezo_device_init()`
- `ezo_send_command()`
- `ezo_send_command_with_float()`
- `ezo_send_read()`
- `ezo_send_read_with_temp_comp()`
- `ezo_read_response()`
- `ezo_read_response_raw()`

I2C transport contract:

- `write_then_read(context, address, tx_data, tx_len, rx_data, rx_len, rx_received)`

I2C Arduino adapter contract:

- wraps `TwoWire`
- remains a thin transport shim
- does not own timing, parsing, or retries

I2C response semantics:

- first byte is the device status byte
- text and raw response reads are explicit separate paths
- valid but unsuccessful device statuses still return `EZO_OK`

I2C status-byte mapping:

- `1` -> `EZO_STATUS_SUCCESS`
- `2` -> `EZO_STATUS_FAIL`
- `254` -> `EZO_STATUS_NOT_READY`
- `255` -> `EZO_STATUS_NO_DATA`

## UART Surface

The UART API provides:

- device init
- generic command send helpers
- read helpers for plain read and read-with-temperature-compensation
- CR-terminated text response reads
- optional explicit input discard
- Arduino `Stream` adapter surface

Primary UART C entry points:

- `ezo_uart_device_init()`
- `ezo_uart_send_command()`
- `ezo_uart_send_command_with_float()`
- `ezo_uart_send_read()`
- `ezo_uart_send_read_with_temp_comp()`
- `ezo_uart_read_response()`
- `ezo_uart_discard_input()`

UART transport contract:

- `write_bytes(context, tx_data, tx_len)`
- `read_bytes(context, rx_data, rx_len, rx_received)`
- optional `discard_input(context)`

UART Arduino adapter contract:

- wraps `Stream`
- reports only currently available bytes to the core
- keeps CR framing policy in the core
- does not use `String`
- does not hide delays

UART framing rules:

- public send helpers accept command text without terminators
- the core appends a single `\r`
- responses are read until a single `\r`
- returned buffers are null-terminated on success
- `response_len` excludes the null terminator

UART response classification:

- `EZO_UART_RESPONSE_DATA`: any successful non-empty line that is not a control token
- `EZO_UART_RESPONSE_OK`: exact `*OK`
- `EZO_UART_RESPONSE_ERROR`: exact `*ER`
- `EZO_UART_RESPONSE_UNKNOWN`: initial or failure state

Rules:

1. `*OK` and `*ER` are device responses, not transport errors.
2. A valid `*ER` response still returns `EZO_OK`; callers inspect the response kind.
3. Zero-length or incomplete lines return `EZO_ERR_PROTOCOL`.
4. Buffer exhaustion before `\r` returns `EZO_ERR_BUFFER_TOO_SMALL`.
5. v1 does not expose a raw UART response API.
6. v1 does not expose a UART C++ wrapper.

## Validation Boundaries

Current validation covers:

- I2C core behavior with fake transports
- UART core behavior with fake transports
- Linux I2C adapter behavior on host builds
- Arduino I2C and UART example compile validation through PlatformIO

Current gap by design:

- POSIX UART adapter support is not part of the current baseline yet

## Explicit Non-Goals

Not part of the current baseline:

- typed pH/EC/RTD helper APIs
- async/state-machine behavior
- hidden retries or hidden delays
- compatibility with the legacy Atlas API shape
- POSIX UART adapter
- UART C++ wrapper
