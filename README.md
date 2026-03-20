# EZO Driver

Cross-platform Atlas Scientific EZO driver with:

- a platform-agnostic C99 core
- explicit I2C and UART driver families
- a thin C++11 wrapper for the I2C surface
- Arduino `TwoWire` integration for I2C
- Arduino `Stream` integration for UART
- Linux I2C support
- Linux host POSIX UART support
- host-side tests and Arduino compile CI

This repository is a rewrite informed by Atlas Scientific's original reference libraries. Legacy reference code remains under [`_reference/`](./_reference/).

## Status

Current implementation includes:

- shared `ezo.h` surface for results, timing hints, and numeric parsing
- product identity and metadata for the initial six documented EZO families
- complete I2C core with text and raw response decoding
- complete UART core with line-based response handling
- Arduino integrations for both I2C and UART
- Linux I2C integration
- Linux host POSIX UART integration
- focused Arduino and Linux examples
- host-side C and C++ tests plus fake transport coverage
- PlatformIO Arduino compile validation in CI

Current support matrix:

- I2C: C core, I2C C++ wrapper, Arduino `TwoWire`, Linux I2C adapter
- UART: C core, Arduino `Stream`, Linux host POSIX serial adapter
- Product metadata: initial six documented families at the metadata tier
- Shared: host-side tests and Arduino compile validation

## Layout

- `src/`: canonical library root for public headers and Arduino-safe implementation
- `platform/`: host-only platform implementation code not intended for Arduino library builds
- `examples/`: focused Arduino and Linux examples
- `tests/`: host-side tests and fakes
- `docs/`: tracked handoff docs and curated EZO product/protocol notes
- `_reference/`: legacy reference material only

## Build

Host builds and tests use CMake:

```sh
cmake -S . -B build -DEZO_BUILD_TESTS=ON -DEZO_BUILD_LINUX_ADAPTER=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

To build the host-side examples, enable the host adapters explicitly:

```sh
cmake -S . -B build -DEZO_BUILD_EXAMPLES=ON -DEZO_BUILD_LINUX_ADAPTER=ON -DEZO_BUILD_POSIX_UART_ADAPTER=ON
```

## Validation

- host CI builds and runs C and C++ tests for the shared, I2C, UART, and product-metadata paths
- Linux I2C and Linux host POSIX UART adapter behavior are covered by host-side tests
- PlatformIO CI compile-checks Arduino I2C and UART examples for `uno`, `nanoatmega328`, and `esp32dev`
- Arduino IDE validation is manual by design

## Packaging

- `library.properties`: Arduino tooling metadata for the combined library
- `library.json`: PlatformIO metadata for the combined library

## Examples

- [`examples/arduino_smoke/arduino_smoke.ino`](./examples/arduino_smoke/arduino_smoke.ino): minimal I2C C API smoke path
- [`examples/arduino_read/arduino_read.ino`](./examples/arduino_read/arduino_read.ino): I2C C++ wrapper example
- [`examples/arduino_uart_smoke/arduino_uart_smoke.ino`](./examples/arduino_uart_smoke/arduino_uart_smoke.ino): minimal UART C API smoke path
- [`examples/arduino_uart_read/arduino_uart_read.ino`](./examples/arduino_uart_read/arduino_uart_read.ino): UART read flow with explicit timing and parse path
- [`examples/linux_read.c`](./examples/linux_read.c): minimal Linux I2C transport example
- [`examples/linux_uart_read.c`](./examples/linux_uart_read.c): minimal Linux host POSIX UART transport example

## Entry Points

Primary public headers:

- [`src/ezo.h`](./src/ezo.h)
- [`src/ezo_i2c.h`](./src/ezo_i2c.h)
- [`src/ezo_i2c.hpp`](./src/ezo_i2c.hpp)
- [`src/ezo_i2c_arduino_wire.h`](./src/ezo_i2c_arduino_wire.h)
- [`src/ezo_i2c_linux_i2c.h`](./src/ezo_i2c_linux_i2c.h)
- [`src/ezo_product.h`](./src/ezo_product.h)
- [`src/ezo_uart.h`](./src/ezo_uart.h)
- [`src/ezo_uart_posix_serial.h`](./src/ezo_uart_posix_serial.h)
- [`src/ezo_uart_arduino_stream.h`](./src/ezo_uart_arduino_stream.h)

Primary implementation files:

- [`src/ezo.c`](./src/ezo.c)
- [`src/ezo_common.c`](./src/ezo_common.c)
- [`src/ezo_i2c.c`](./src/ezo_i2c.c)
- [`src/ezo_i2c_arduino_wire.cpp`](./src/ezo_i2c_arduino_wire.cpp)
- [`platform/linux/ezo_i2c_linux_i2c.c`](./platform/linux/ezo_i2c_linux_i2c.c)
- [`src/ezo_product.c`](./src/ezo_product.c)
- [`src/ezo_uart.c`](./src/ezo_uart.c)
- [`platform/linux/ezo_uart_posix_serial.c`](./platform/linux/ezo_uart_posix_serial.c)
- [`src/ezo_uart_arduino_stream.cpp`](./src/ezo_uart_arduino_stream.cpp)

## Docs

- [`docs/ezo/README.md`](./docs/ezo/README.md): curated EZO product and protocol context for this repo
- [`docs/architecture.md`](./docs/architecture.md): structure, boundaries, packaging, validation
- [`docs/api-contract.md`](./docs/api-contract.md): behavioral contract for the public API
- [`docs/canonical-library-direction.md`](./docs/canonical-library-direction.md): stable long-term direction for the canonical product-aware library
- [`CHANGELOG.md`](./CHANGELOG.md): tracked change history

## Scope Notes

Intentionally out of scope for the current baseline:

- typed pH/EC/RTD helper APIs
- async/state-machine behavior
- hidden retries or hidden delays
- compatibility with the legacy Atlas API shape
- UART C++ wrapper
