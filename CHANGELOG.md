# Changelog

All notable tracked changes to this rewrite will be recorded here.

## [Unreleased]

### Added

- typed EC, DO, and HUM product modules over both I2C and UART
- explicit output-configuration helpers for the multi-output product families
- EC temperature/probe-K/TDS-factor helpers, DO temperature/salinity/pressure helpers, and focused Linux I2C examples for EC/DO/HUM
- host-side fake-transport tests for EC, DO, and HUM

### Changed

- promoted EC, DO, and HUM metadata support tiers to typed-read support
- aligned the HUM canonical schema order with the product's humidity, air-temperature, then dew-point output order
- updated the tracked docs and support matrix to reflect full Phase 5 typed coverage

## [0.3.0] - current baseline

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
- `_reference/` remains legacy reference material and is not part of the product surface
