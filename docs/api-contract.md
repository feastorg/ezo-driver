# API Contract

Status: accepted

Source of truth:

- shared C API: `src/ezo.h`
- DO product API: `src/ezo_do.h`
- EC product API: `src/ezo_ec.h`
- HUM product API: `src/ezo_hum.h`
- shared parse API: `src/ezo_parse.h`
- shared schema API: `src/ezo_schema.h`
- I2C C API: `src/ezo_i2c.h`
- I2C C++ API: `src/ezo_i2c.hpp`
- I2C Arduino adapter API: `src/ezo_i2c_arduino_wire.h`
- ORP product API: `src/ezo_orp.h`
- pH product API: `src/ezo_ph.h`
- product metadata API: `src/ezo_product.h`
- RTD product API: `src/ezo_rtd.h`
- UART C API: `src/ezo_uart.h`
- POSIX UART adapter API: `src/ezo_uart_posix_serial.h`
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

## Product Surface

The product metadata API provides:

- product IDs for the initial six documented families
- static registry lookup for defaults, capabilities, and support tiers
- device-info parsing for `i` responses
- product-aware timing lookup by transport and command kind

Primary product entry points:

- `ezo_product_id_from_short_code()`
- `ezo_parse_device_info()`
- `ezo_product_get_metadata()`
- `ezo_product_get_metadata_by_short_code()`
- `ezo_product_get_timing_hint()`
- `ezo_product_resolve_timing_hint()`
- `ezo_product_get_support_tier()`
- `ezo_product_supports_capability()`
- `ezo_product_has_command_family()`

Product metadata rules:

1. The registry is static and hand-authored.
2. Product identity stays separate from the I2C and UART device structs.
3. Syntactically valid but unsupported `i` responses parse successfully and map to `EZO_PRODUCT_UNKNOWN`.
4. Default UART-state metadata is bootstrapping guidance only; higher layers still verify or configure runtime state when determinism matters.
5. Firmware-sensitive defaults may be recorded as query-required instead of as a guessed fact.
6. The metadata layer is facts and lookups only; typed product helpers live in separate product modules.
7. `ezo_product_resolve_timing_hint()` prefers product-specific timing when the metadata can answer the request and otherwise falls back to the shared command-kind hint.

## Shared Parse And Schema Surface

The shared parse/schema layer provides:

- borrowed text spans for non-owning field views
- CSV and `?Prefix,...` query parsing helpers
- a small UART sequence state helper above one-line reads
- canonical output-schema descriptors for the initial six products
- scalar and multi-output reading structs

Primary shared entry points:

- `ezo_text_span_t`
- `ezo_parse_text_span_double()`
- `ezo_parse_csv_fields()`
- `ezo_parse_query_response()`
- `ezo_parse_prefixed_fields()`
- `ezo_uart_sequence_init()`
- `ezo_uart_sequence_push_line()`
- `ezo_uart_sequence_is_complete()`
- `ezo_schema_get_output_schema()`
- `ezo_schema_count_enabled_fields()`
- `ezo_schema_parse_scalar_reading()`
- `ezo_schema_parse_multi_output_reading()`

Rules:

1. Text spans borrow caller-owned buffers and are not null-terminated copies.
2. `ezo_parse_query_response()` only handles the shared `?Prefix,...` response shape; it is not a universal parser for every device response.
3. Query and CSV helpers trim surrounding ASCII whitespace on each field.
4. Empty CSV fields are preserved as zero-length spans instead of being discarded.
5. `ezo_uart_sequence_t` tracks sequence state only; it does not read from transports or interpret product-specific workflow meaning.
6. Output schemas encode canonical field order, not guaranteed runtime configuration.
7. Multi-output parsing requires an explicit enabled-field mask from the caller or higher layer.

## Product Module Surface

The product-module layer currently provides:

- typed pH helpers in `src/ezo_ph.h`
- typed ORP helpers in `src/ezo_orp.h`
- typed RTD helpers in `src/ezo_rtd.h`
- typed EC helpers in `src/ezo_ec.h`
- typed DO helpers in `src/ezo_do.h`
- typed HUM helpers in `src/ezo_hum.h`

Common shape:

- parse helpers for typed readings and shared query forms
- command builders for product-specific setters or calibration commands
- explicit I2C send/read helpers
- explicit UART send/read helpers

Rules:

1. Product modules stay transport-explicit; there is no unified product device object.
2. Send helpers return timing hints but do not sleep.
3. Typed read/query helpers assume the device returned the expected success payload shape; callers that need raw status distinctions still use the transport-level APIs directly.
4. UART helpers may consume more than one line when a product response sequence requires it.
5. RTD reading helpers require the caller to provide the current temperature scale unless that scale was queried separately first.
6. Multi-output typed reading helpers require an explicit enabled-output mask from the caller and do not hide an output-configuration query internally.
7. EC, DO, and HUM output-configuration helpers are product-specific; the shared parse/schema layer does not expose a public generic output-config parser.
8. Current Phase 5 typed coverage for EC/DO/HUM is limited to typed reads, output selection, and measurement-critical compensation/configuration helpers; broader control/admin and advanced calibration-transfer work remains separate.

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
- send helpers clear the cached last status to `EZO_STATUS_UNKNOWN` before a new command
- read helpers update the cached last status from the decoded status byte

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
- POSIX serial adapter surface
- Arduino `Stream` adapter surface

Primary UART C entry points:

- `ezo_uart_device_init()`
- `ezo_uart_send_command()`
- `ezo_uart_send_command_with_float()`
- `ezo_uart_send_read()`
- `ezo_uart_send_read_with_temp_comp()`
- `ezo_uart_read_line()`
- `ezo_uart_read_response()`
- `ezo_uart_response_kind_is_control()`
- `ezo_uart_response_kind_is_terminal()`
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

POSIX UART adapter contract:

- owns the file descriptor it opens
- requires explicit baud selection
- configures termios for 8N1, no flow control, and bounded reads
- restores saved termios state on close
- exposes the standard `ezo_uart_transport_t` through a transport getter

UART framing rules:

- public send helpers accept command text without terminators
- the core appends a single `\r`
- `ezo_uart_read_line()` reads one CR-terminated line
- `ezo_uart_read_response()` is a compatibility wrapper around that same one-line primitive
- returned buffers are null-terminated on success
- `response_len` excludes the null terminator

UART response classification:

- `EZO_UART_RESPONSE_DATA`: any successful non-empty line that is not a control token
- `EZO_UART_RESPONSE_OK`: exact `*OK`
- `EZO_UART_RESPONSE_ERROR`: exact `*ER`
- `EZO_UART_RESPONSE_OVER_VOLTAGE`: exact `*OV`
- `EZO_UART_RESPONSE_UNDER_VOLTAGE`: exact `*UV`
- `EZO_UART_RESPONSE_RESET`: exact `*RS`
- `EZO_UART_RESPONSE_READY`: exact `*RE`
- `EZO_UART_RESPONSE_SLEEP`: exact `*SL`
- `EZO_UART_RESPONSE_WAKE`: exact `*WA`
- `EZO_UART_RESPONSE_DONE`: exact `*DONE`
- `EZO_UART_RESPONSE_UNKNOWN`: initial or failure state

Rules:

1. `*OK` and `*ER` are device responses, not transport errors.
2. A valid `*ER` response still returns `EZO_OK`; callers inspect the response kind.
3. The low-level UART primitive reads one line, not an entire command-response sequence.
4. Multi-line sequences are caller-owned or higher-layer-owned flows built on repeated line reads.
5. `ezo_uart_response_kind_is_control()` identifies non-data control/status tokens.
6. `ezo_uart_response_kind_is_terminal()` identifies line kinds that can complete a sequence without implying that all sequences are one line long.
7. Startup or power-state tokens such as `*WA`, `*RE`, and `*RS` are surfaced as valid control events; the core does not hide them.
8. Higher layers that need a clean workflow boundary should consume or discard stale continuous output and trailing status lines before assuming the next line belongs to a new command.
9. Shipping defaults such as continuous mode enabled or `*OK` enabled are only bootstrapping heuristics; deterministic higher layers should verify or configure the mode they depend on.
10. `ezo_uart_discard_input()` is the explicit resynchronization tool when a caller abandons a sequence or wants to drop stale input.
11. Zero-length or incomplete lines return `EZO_ERR_PROTOCOL`.
12. Buffer exhaustion before `\r` returns `EZO_ERR_BUFFER_TOO_SMALL`.
13. v1 does not expose a raw UART response API.
14. v1 does not expose a UART C++ wrapper.

## Validation Boundaries

Current validation covers:

- I2C core behavior with fake transports
- product metadata and device-info parsing with host-side tests
- shared parse, schema, and timing-resolution behavior with host-side tests
- typed product helpers for pH, ORP, RTD, EC, DO, and HUM with host-side fake-transport tests
- UART core behavior with fake transports
- Linux I2C and Linux host POSIX UART adapter behavior on host builds
- Arduino I2C and UART example compile validation through PlatformIO

Current gap by design:

- a UART C++ wrapper is not part of the current baseline yet

## Explicit Non-Goals

Not part of the current baseline:

- broad typed control-plane workflows
- advanced per-product helpers such as calibration transfer and HUM temperature calibration
- async/state-machine behavior
- hidden retries or hidden delays
- compatibility with the legacy Atlas API shape
- UART C++ wrapper
