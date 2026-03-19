# Error And Status Model

Status: Accepted for header scaffolding
Phase: 01

## Purpose

Define how the driver reports failures and device state without conflating unrelated concerns.

## Model Overview

The driver has two different categories of outcome:

1. library result
2. device-reported status

They must remain separate.

## Library Result

The library result reports whether the driver operation itself completed correctly.

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

### Meaning

- `EZO_OK`
  The operation completed at the library level.

- `EZO_ERR_INVALID_ARGUMENT`
  Caller passed an invalid pointer, invalid length, or invalid parameter combination.

- `EZO_ERR_BUFFER_TOO_SMALL`
  Caller-provided buffer cannot hold the required output or cannot safely represent the response.

- `EZO_ERR_TRANSPORT`
  The underlying transport callback failed.

- `EZO_ERR_PROTOCOL`
  The bus transaction completed, but the returned bytes are not valid for the protocol contract.

- `EZO_ERR_PARSE`
  The payload was received successfully, but parsing into the requested representation failed.

- `EZO_ERR_STATE`
  The caller requested an operation that violates documented API state rules.

## Device Status

The device status reports what the EZO circuit itself reported.

```c
typedef enum {
  EZO_STATUS_UNKNOWN = 0,
  EZO_STATUS_SUCCESS,
  EZO_STATUS_FAIL,
  EZO_STATUS_NOT_READY,
  EZO_STATUS_NO_DATA
} ezo_device_status_t;
```

## Reference Mapping

The old library implicitly maps:

- `1 -> success`
- `2 -> fail`
- `254 -> not ready`
- `255 -> no data`

The new core should keep this mapping, but treat unknown values as protocol-level problems rather than silently reusing stale state.

## Interpretation Rules

### Case 1: transport failed

- library result: `EZO_ERR_TRANSPORT`
- device status: unchanged or set to `EZO_STATUS_UNKNOWN`

### Case 2: invalid or unknown status byte

- library result: `EZO_ERR_PROTOCOL`
- device status: `EZO_STATUS_UNKNOWN`

### Case 3: valid status byte with unsuccessful device status

Examples:

- `EZO_STATUS_FAIL`
- `EZO_STATUS_NOT_READY`
- `EZO_STATUS_NO_DATA`

In this case:

- library result: `EZO_OK`
- device status: one of the above values

This matters because the transaction and protocol decode succeeded even though the device did not return a successful payload.

### Case 4: successful response but parse failure

- library result: `EZO_ERR_PARSE`
- device status: `EZO_STATUS_SUCCESS`

This distinguishes "device responded successfully" from "caller requested a parse that failed."

## State Rules

The device object may cache the last known device status for diagnostic convenience, but APIs that produce a status should also return it directly so callers do not need to rely on hidden mutable state.

## Canonical Handling Pattern

```c
ezo_result_t result;
ezo_device_status_t status;

result = ezo_read_response(..., &status);

if (result != EZO_OK) {
  /* library or transport failure */
}

if (status != EZO_STATUS_SUCCESS) {
  /* device-level unsuccessful response */
}
```

## Design Rule

The core must never report a device status failure as a transport failure, and must never report a transport failure as a device status.
