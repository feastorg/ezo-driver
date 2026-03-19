# Reference Audit

Status: Accepted input to Phase 01 contract locking

## Purpose

Record the specific findings taken from `_reference/` that matter to the new API contracts.

## Findings

### 1. The legacy API shape is not the protocol shape

The reference library mixes protocol behavior with Arduino-specific object workflow.

Example:

- `issued_read`
- `receive_read_cmd()`
- `NOT_READ_CMD`

These are API-level workflow constraints, not wire-level requirements.

Conclusion:

- the new core should not preserve this state model unless it buys something concrete

### 2. Device status byte mapping is stable

The reference core maps:

- `1 -> success`
- `2 -> fail`
- `254 -> not ready`
- `255 -> no data`

Conclusion:

- preserve this mapping in the new core
- treat unknown status bytes as protocol errors rather than stale state reuse

### 3. Generic command timing is usually treated as `300 ms`

The reference helper logic in `_reference/iot_cmd.cpp` uses `300 ms` for many non-read, non-calibration commands.

Conclusion:

- `300 ms` is a reasonable conservative generic timing hint for v1

### 4. Read timing is inconsistent but bounded conservatively

The reference repository uses multiple read timing patterns:

- `1000 ms` in common polling examples
- `815 ms` in some RTD and temperature-compensated read examples
- `1200 ms` in helper code for broad `R` and `CAL` classification

Conclusion:

- v1 should use conservative class-level timing hints rather than pretend there is one exact universal per-command truth in the reference material

### 5. Numeric command formatting is part of the protocol surface

The reference library has generic numeric command helpers:

- `send_cmd_with_num()`
- `send_read_with_temp_comp()`

Conclusion:

- numeric-formatting helpers belong in the new generic API
- they do not require typed device-specific wrappers

### 6. Text response parsing is a real first-class need

The reference examples rely on raw text responses for:

- basic response printing
- numeric parsing
- multi-field parsing examples such as humidity and EC response handling

Conclusion:

- the first API must support raw text response retrieval explicitly
- typed parsing helpers can remain narrow in v1

## Contract Impact

These findings support the locked Phase 01 choices:

- generic API
- explicit timing
- separate error and status model
- single small transport boundary
- no legacy read-issued compatibility state
