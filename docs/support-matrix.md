# Support Matrix

## Purpose

This document is the public support statement for the repo.

## Support Tier Policy

`METADATA`

- product identity
- timing metadata
- canonical schema facts where applicable
- no claim of typed measurement coverage

`TYPED_READ`

- typed measurement reads over the supported transports
- product-specific parse and command helpers needed for the main measurement path
- partial configuration coverage is acceptable

`FULL`

- typed reads
- shared control/admin coverage where the product participates
- calibration-transfer coverage where the product supports it
- advanced product-specific helpers that are part of the documented public surface for the initial family

Support tiers are repo claims, not vendor claims. A product moves tiers only when the implementation, tests, and docs all match.

Future family onboarding follows [`docs/product-onboarding.md`](./product-onboarding.md).

## Product Matrix

| Product | Tier | Typed Reads | Shared Control | Calibration Transfer | Product-Specific Advanced Surface |
| --- | --- | --- | --- | --- | --- |
| pH | `FULL` | yes | yes | yes | temperature compensation, calibration, slope, extended range |
| ORP | `FULL` | yes | yes | yes | calibration, extended range |
| EC | `FULL` | yes | yes | yes | output config, temperature, probe `K`, TDS factor, calibration |
| DO | `FULL` | yes | yes | yes | output config, temperature, salinity, pressure, calibration |
| RTD | `FULL` | yes | yes | yes | scale, logger, sequential memory, bulk memory, calibration |
| HUM | `FULL` | yes | yes | no | output config, temperature calibration |

## Transport And Platform Matrix

| Surface | Status | Notes |
| --- | --- | --- |
| C I2C core | supported | public C API |
| C UART core | supported | public C API |
| I2C C++ wrapper | supported | thin convenience layer only |
| UART C++ wrapper | deferred | intentionally not shipped |
| Arduino `TwoWire` adapter | supported | compile-validated in CI |
| Arduino `Stream` adapter | supported | compile-validated in CI |
| Linux I2C adapter | supported | host build/test path |
| Linux POSIX UART adapter | supported | host build/test path |

## Important Non-Goals

- async/state-machine workflows
- hidden reconnect after reboot, sleep, or mode switching
- hidden retries or hidden timing delays
- a fake unified device abstraction across I2C and UART

## Validation Expectations Behind The Matrix

The public matrix assumes:

- host-side fake-transport tests for shared and typed APIs
- tracked docs that describe the same surface the code exports
- transport/platform claims limited to the paths the repo actually builds and validates
