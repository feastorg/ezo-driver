# Transport Contract

Status: Accepted for header scaffolding
Phase: 01

## Purpose

Define the boundary between the platform-agnostic core and platform-specific I2C implementations.

## Design Goals

- small surface
- no platform types in the core
- sufficient for Arduino and Linux
- easy to fake in tests

## Contract Shape

The transport is a function table plus caller-owned context.

```c
typedef struct ezo_i2c_transport {
  ezo_result_t (*write_then_read)(
      void *context,
      uint8_t address,
      const uint8_t *tx_data,
      size_t tx_len,
      uint8_t *rx_data,
      size_t rx_len,
      size_t *rx_received);
} ezo_i2c_transport_t;
```

## Why A Single Transaction Primitive

The reference behavior is effectively:

1. write a command to the device
2. later read a response from the device

Those happen at different times from the caller's point of view, but each individual transport operation is still a bounded bus transaction.

Using one primitive for "write some bytes, optionally read some bytes" keeps the core small:

- send-only calls set `rx_data = NULL`, `rx_len = 0`
- read-only calls set `tx_data = NULL`, `tx_len = 0`

This avoids needing multiple primitives unless implementation pressure proves otherwise.

## Operation Semantics

### Send-only command

- `tx_data` must be non-null when `tx_len > 0`
- `rx_data` may be null when `rx_len == 0`
- `rx_received` should be set to `0` if provided

### Read-only response fetch

- `tx_data` may be null when `tx_len == 0`
- `rx_data` must be non-null when `rx_len > 0`
- `rx_received` returns the number of bytes actually read

### Mixed transaction

Not required by the first protocol flow, but supported by the contract for completeness if a platform can do it safely.

## Transport Responsibilities

The transport layer is responsible for:

- performing the bus transaction
- returning transport-level success or failure
- reporting how many bytes were read

The transport layer is not responsible for:

- parsing EZO response payloads
- interpreting EZO status bytes
- applying protocol timing rules
- deciding command classes

## Core Responsibilities Above The Transport

The core is responsible for:

- deciding what bytes to send
- deciding how many bytes to request
- interpreting returned status bytes
- parsing returned payloads
- exposing timing requirements to the caller

## Transport Error Mapping

The callback returns `ezo_result_t`.

Expected callback return values:

- `EZO_OK`
- `EZO_ERR_INVALID_ARGUMENT`
- `EZO_ERR_TRANSPORT`

Other library-level results should normally be produced by the core rather than the transport.

## Adapter Expectations

### Arduino adapter

- wraps `TwoWire`
- translates Arduino write/read mechanics into the transport contract
- does not leak Arduino types into public core headers

### Linux adapter

- wraps the chosen Linux I2C mechanism
- translates Linux transaction behavior into the same contract

### Fake transport

- deterministic
- programmable by tests
- able to model nominal, short-read, invalid-status, and transport-failure cases

## Buffer Rules

- transport must never write beyond `rx_len`
- transport must set `rx_received` to the actual byte count if provided
- short reads are allowed and must be reported accurately
- the core must not assume the buffer is null-terminated

## Revision Rule

If later adapter implementation shows that Arduino and Linux integrations are materially cleaner with separate `write()` and `read()` callbacks, this document should be revised explicitly in the decision log and architecture docs before code diverges.
