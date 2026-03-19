# Timing Semantics

Status: Accepted for header scaffolding
Phase: 01

## Purpose

Define how command timing works in the new core without hiding sleeps inside the library.

## Core Rule

The core does not sleep.

The core exposes timing requirements, and the caller decides how to wait:

- blocking sleep
- cooperative scheduler
- event loop
- polling loop

## Reference Behavior

The reference code effectively uses:

- around `300 ms` for many generic commands
- around `815-1000 ms` for many read paths in examples
- around `1200 ms` in helper code for broad `R` and `CAL` categories

The exact policy in the new driver should be explicit and centralized rather than scattered through examples.

## Timing Contract

Command submission functions may return a timing hint:

```c
typedef struct {
  uint32_t wait_ms;
} ezo_timing_hint_t;
```

If a timing hint is requested, the function must populate it deterministically based on the command kind.

## Timing Categories

Initial categories:

- `EZO_COMMAND_GENERIC`
- `EZO_COMMAND_READ`
- `EZO_COMMAND_READ_WITH_TEMP_COMP`
- `EZO_COMMAND_CALIBRATION`

## Initial Timing Policy

The v1 driver will use conservative generic timing hints:

- generic command: `300 ms`
- read command: `1000 ms`
- read with temperature compensation: `1000 ms`
- calibration command: `1200 ms`

These are class-level defaults, not per-device guarantees. The reference repo shows some examples successfully using shorter read windows such as `815 ms`, but v1 will prefer conservative defaults until device-specific timing policy is intentionally added.

If later protocol evidence shows a need for different timing classes, revise the categories explicitly rather than letting examples drift.

## Caller Responsibilities

After a successful send call, the caller:

1. receives the timing hint
2. waits according to its own execution model
3. calls the appropriate response-read function

## Library Responsibilities

The library:

- classifies the command kind
- returns the documented timing hint
- does not block internally

The library does not:

- call `delay()`
- call `sleep()`
- perform retries implicitly

## Optional Convenience Helpers

Platform-specific or utility-layer helpers may exist later to make waiting easier, but they must remain outside the core.

Examples:

- Arduino helper that wraps `delay()`
- Linux helper that wraps `nanosleep()`

Those helpers must not become required for correct use of the core API.

## Canonical Flow

```c
ezo_timing_hint_t hint;

ezo_send_read(&device, &hint);
/* caller waits hint.wait_ms */
ezo_read_response(...);
```

## Design Rule

Timing semantics belong to the documented contract, not to examples, adapters, or hidden implementation delays.
