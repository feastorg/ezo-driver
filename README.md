# EZO I2C Driver

Small cross-platform EZO I2C driver rewrite with:

- a platform-agnostic C99 core
- a thin C++11 wrapper
- Arduino `TwoWire` integration
- Linux I2C support
- host-side tests and Arduino compile CI

This repository is a rewrite informed by Atlas Scientific's original `Ezo_i2c_lib`. The legacy reference code remains under [`_reference/`](./_reference/).

## Status

Current implementation includes:

- generic EZO command send/read flow
- text and raw response decoding with numeric parsing helpers
- Arduino and Linux platform integrations
- Arduino examples for both the C and C++ surfaces
- CMake host build/test flow
- PlatformIO Arduino compile validation in CI

## Layout

- `src/`: canonical library root for public headers and implementation
- `platform/`: host-only platform implementation code not intended for Arduino library builds
- `examples/`: focused Arduino and Linux examples
- `tests/`: host-side tests and fakes
- `docs/`: tracked handoff docs
- `_reference/`: legacy reference material only

## Build

Host builds and tests use CMake:

```sh
cmake -S . -B build -DEZO_BUILD_TESTS=ON -DEZO_BUILD_LINUX_ADAPTER=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Validation

- host CI builds and runs C and C++ tests
- Linux adapter behavior is covered by host-side tests
- PlatformIO CI compile-checks Arduino examples for `uno`, `nanoatmega328`, and `esp32dev`
- Arduino IDE validation is manual by design

## Packaging

- `library.properties`: Arduino tooling metadata
- `library.json`: PlatformIO metadata

## Examples

- [`examples/arduino_smoke/arduino_smoke.ino`](./examples/arduino_smoke/arduino_smoke.ino): minimal C API smoke path
- [`examples/arduino_read/arduino_read.ino`](./examples/arduino_read/arduino_read.ino): C++ wrapper example
- [`examples/linux_read.c`](./examples/linux_read.c): minimal Linux transport example

## Entry Points

Primary public headers:

- [`src/ezo_i2c.h`](./src/ezo_i2c.h)
- [`src/ezo_i2c.hpp`](./src/ezo_i2c.hpp)
- [`src/ezo_i2c_arduino_wire.h`](./src/ezo_i2c_arduino_wire.h)

Primary implementation files:

- [`src/ezo_i2c.c`](./src/ezo_i2c.c)
- [`src/arduino/ezo_arduino_wire.cpp`](./src/arduino/ezo_arduino_wire.cpp)
- [`platform/linux/ezo_linux_i2c.c`](./platform/linux/ezo_linux_i2c.c)

## Docs

- [`docs/architecture.md`](./docs/architecture.md): structure, boundaries, packaging, validation
- [`docs/api-contract.md`](./docs/api-contract.md): behavioral contract for the public API
- [`CHANGELOG.md`](./CHANGELOG.md): tracked change history

## Scope Notes

Intentionally out of scope for v1:

- typed pH/EC/RTD helper APIs
- async/state-machine behavior
- hidden retries or hidden delays
- compatibility with the legacy Atlas API shape
