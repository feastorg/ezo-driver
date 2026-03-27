# Changelog

All notable tracked changes to this rewrite will be recorded here.

## [Unreleased]

## [0.5.1]

### Added

- broadened the canonical Linux example surface across staged calibration, product workflow, shared control, transport switching, calibration transfer, and cross-device compensation flows for the initial six products
- broadened the maintained Arduino example surface to include full typed coverage plus curated advanced I2C and UART workflows

### Changed

- retired the legacy `_reference/` tree in favor of the maintained canonical examples under `examples/linux/` and `examples/arduino/`
- aligned the public support/docs story so the README and support matrix explicitly include the Linux Python bindings support surface
- cleaned repo hygiene by removing tracked Python build artifacts from `bindings/python/src/` and ignoring them going forward

### Fixed

- fixed Arduino PlatformIO example builds by moving shared Arduino helper headers onto the library include path instead of sketch-relative include paths
- fixed the Arduino UART `multi_device_router` sketch for Uno builds by making its routed-module type safe for the Arduino auto-prototype pass

## [0.5.0]

### Fixed

- accept observed uppercase DO output-query payloads such as `?O,MG` in typed output-config parsing
- accept DO output-query variants with explicit enable flags such as `?O,MG,1,%,0`, enabling only the marked outputs
- add DO unit coverage for uppercase and flag-bearing output-query variants to prevent regressions

## [0.4.2]

### Fixed

- accept observed uppercase shared control responses such as `?STATUS,P,4.91` in shared control parsing
- keep the Arduino I2C inspect example on the normal helper path after the real-hardware parser fixes, while retaining the safer startup settle and pending-response drain

## [0.4.1]

### Fixed

- accept observed uppercase identity responses such as `?I,DO,2.16` in shared device-info parsing
- harden the Arduino I2C inspect example for real hardware startup by using a longer settle window and draining pending responses before probing identity

## [0.4.0] - current baseline

### Added

- shared `ezo_control` module for info, name, status, LED, UART response-code mode, sleep, factory reset, protocol lock, and mode-switch helpers
- shared `ezo_calibration_transfer` module for export/import workflows
- advanced typed product helpers for pH extended range, RTD logger/memory, and HUM temperature calibration
- typed calibration helpers for pH, EC, DO, and RTD
- host-side fake-transport tests for the shared control/calibration-transfer modules and the expanded product helpers
- public API layer and support-matrix docs
- canonical Linux examples for device identity and shared control queries
- tracked product-onboarding doc for future family additions

### Changed

- promoted the initial six documented product families to full support
- aligned the HUM canonical schema order with the product's humidity, air-temperature, then dew-point output order
- completed RTD bulk memory recall support and richer I2C calibration-import completion reporting
- updated the tracked docs to reflect full Phase 6 control-plane and advanced-feature coverage
- refined package metadata and added a CMake install/export surface for host consumers
- explicitly deferred the UART C++ wrapper again during Phase 7 public-surface consolidation
- removed the unused `TRANSPORT_BASELINE` support tier and closed out product-onboarding guidance in tracked docs

## [0.3.0]

### Added

- POSIX UART public header and Linux host adapter
- minimal Linux UART example
- host-side POSIX UART adapter tests

### Changed

- closed the remaining host parity gap so both I2C and UART now have Arduino and Linux host paths

### Notes

- current support matrix is now I2C = Arduino + Linux and UART = Arduino + Linux host POSIX serial
- a UART C++ wrapper is still intentionally deferred

## [0.2.0] - multi-mode baseline

### Added

- UART reference material under `_reference/Ezo_uart_lib/` to anchor the new mode against the original Atlas behavior
- shared formatting and parsing helpers in `ezo_common`
- shared public `ezo.h` surface for result types, timing hints, and numeric parsing
- UART C core with line-based response framing and response classification
- Arduino `Stream` transport adapter for UART
- focused Arduino UART smoke and read examples
- host-side fake UART transport coverage and UART core tests

### Changed

- repositioned the repo and tracked docs around an explicit multi-mode `ezo-driver` surface
- aligned CMake and package metadata with the multi-mode product scope
- expanded PlatformIO Arduino compile CI to include UART examples

### Notes

- UART platform support in this release means Arduino `Stream`
- POSIX UART support and a UART C++ wrapper are deferred beyond this release

## [0.1.0] - initial I2C baseline

### Added

- C99 transport-agnostic I2C core for EZO command formatting, response decoding, and numeric helpers
- thin header-only C++11 wrapper over the I2C C core
- Arduino `TwoWire` transport adapter for I2C
- Linux file-descriptor transport adapter for I2C
- raw response API alongside the I2C text response path
- focused Arduino and Linux examples for the I2C path
- host-side C and C++ tests with fake transport coverage
- Linux adapter behavior tests
- GitHub Actions CI for host builds/tests
- PlatformIO Arduino compile CI for `uno`, `nanoatmega328`, and `esp32dev`

### Changed

- simplified the repo so `src/` is the canonical library root for public headers and Arduino-safe implementation
- reduced tracked docs to a small handoff-oriented set

### Notes

- Arduino IDE validation is manual only
- `_reference/` remains repo-internal reference material and is not part of the product surface
