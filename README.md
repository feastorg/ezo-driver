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

## Status

Current implementation includes:

- shared `ezo.h` surface for results, timing hints, and numeric parsing
- product identity and metadata for the initial six documented EZO families
- shared query/CSV parsing, UART sequence-state helpers, and canonical output schemas
- shared control-plane helpers for identity, status, LED, UART response-code mode, sleep, factory reset, protocol lock, and mode switching
- shared calibration-transfer helpers for export/import workflows
- typed scalar product modules for pH, ORP, and RTD
- typed multi-output product modules for EC, DO, and HUM
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
- Product modules: full typed support for the initial six families, including shared control/admin coverage, calibration-transfer primitives, and advanced per-product helpers
- Product foundation: identity, metadata, timing fallback, and parse/schema utilities for the initial six documented families
- Shared: host-side tests and Arduino compile validation

## Start Here

- Raw transport bring-up: [`src/ezo_i2c.h`](./src/ezo_i2c.h) or [`src/ezo_uart.h`](./src/ezo_uart.h)
- Device identity and support metadata: [`src/ezo_control.h`](./src/ezo_control.h) plus [`src/ezo_product.h`](./src/ezo_product.h)
- Shared admin/protocol workflows: [`src/ezo_control.h`](./src/ezo_control.h)
- Calibration export/import primitives: [`src/ezo_calibration_transfer.h`](./src/ezo_calibration_transfer.h)
- Known-product application code: the matching typed product header in `src/`

Public guidance docs:

- [`docs/public-api-layers.md`](./docs/public-api-layers.md)
- [`docs/support-matrix.md`](./docs/support-matrix.md)
- [`docs/product-onboarding.md`](./docs/product-onboarding.md)
- [`docs/examples.md`](./docs/examples.md)

## Layout

- `src/`: canonical library root for public headers and Arduino-safe implementation
- `platform/`: host-only platform implementation code not intended for Arduino library builds
- `examples/linux/`: full Linux reference example tree with `raw`, `commissioning`, `typed`, and `advanced` flows for I2C and UART
- `examples/arduino/`: curated Arduino smoke, commissioning, and simple typed sketches
- `tests/`: host-side tests and fakes
- `docs/`: tracked handoff docs and curated EZO product/protocol notes

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

To install the public headers and static libraries for host-side consumption:

```sh
cmake -S . -B build
cmake --build build
cmake --install build --prefix <install-prefix>
```

## Validation

- host CI builds and runs C and C++ tests for the shared, parse/schema, I2C, UART, and product-foundation paths
- host CI also runs typed product-module tests for pH, ORP, RTD, EC, DO, and HUM
- Linux I2C and Linux host POSIX UART adapter behavior are covered by host-side tests
- PlatformIO CI compile-checks the curated Arduino I2C and UART sketches for `uno`, `nanoatmega328`, and `esp32dev`
- Arduino IDE validation is manual by design

## Packaging

- `library.properties`: Arduino tooling metadata for the combined library
- `library.json`: PlatformIO metadata for the combined library

## Examples

Start with the example chooser in [`docs/examples.md`](./docs/examples.md). That doc is the canonical matrix and first-use guide.

Short version:

- If transport is unknown, start with [`examples/linux/uart/commissioning/inspect_device.c`](./examples/linux/uart/commissioning/inspect_device.c). UART is the shipping default across the currently documented products.
- If transport is known but setup state is not, start with the matching readiness example:
  [`examples/linux/i2c/commissioning/readiness_check.c`](./examples/linux/i2c/commissioning/readiness_check.c)
  or [`examples/linux/uart/commissioning/readiness_check.c`](./examples/linux/uart/commissioning/readiness_check.c)
- If product and transport are already known, jump straight to the matching typed example under `examples/linux/<transport>/typed/`.
- If you need the bare transport contract first, start with:
  [`examples/linux/i2c/raw/raw_command.c`](./examples/linux/i2c/raw/raw_command.c)
  or [`examples/linux/uart/raw/raw_command.c`](./examples/linux/uart/raw/raw_command.c)
- If you need Arduino, use the curated sketches under `examples/arduino/`:
  raw smoke, commissioning inspect, simple pH and D.O. reads for both I2C and UART,
  plus RTD-driven EC temperature compensation and hardware-topology-specific UART routing.

Representative entry points:

- Linux I2C raw: [`examples/linux/i2c/raw/raw_command.c`](./examples/linux/i2c/raw/raw_command.c)
- Linux UART raw: [`examples/linux/uart/raw/raw_command.c`](./examples/linux/uart/raw/raw_command.c)
- Linux I2C commissioning: [`examples/linux/i2c/commissioning/inspect_device.c`](./examples/linux/i2c/commissioning/inspect_device.c)
- Linux UART commissioning: [`examples/linux/uart/commissioning/inspect_device.c`](./examples/linux/uart/commissioning/inspect_device.c)
- Linux I2C typed pH: [`examples/linux/i2c/typed/read_ph.c`](./examples/linux/i2c/typed/read_ph.c)
- Linux UART typed pH: [`examples/linux/uart/typed/read_ph.c`](./examples/linux/uart/typed/read_ph.c)
- Linux advanced calibration transfer:
  [`examples/linux/i2c/advanced/calibration_transfer.c`](./examples/linux/i2c/advanced/calibration_transfer.c)
  and [`examples/linux/uart/advanced/calibration_transfer.c`](./examples/linux/uart/advanced/calibration_transfer.c)
- Linux advanced RTD-driven EC temperature compensation:
  [`examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c`](./examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c)
  and [`examples/linux/uart/advanced/ec_temp_comp_from_rtd.c`](./examples/linux/uart/advanced/ec_temp_comp_from_rtd.c)
- Linux advanced EC-driven D.O. salinity compensation:
  [`examples/linux/i2c/advanced/do_salinity_comp_from_ec.c`](./examples/linux/i2c/advanced/do_salinity_comp_from_ec.c)
  and [`examples/linux/uart/advanced/do_salinity_comp_from_ec.c`](./examples/linux/uart/advanced/do_salinity_comp_from_ec.c)
- Arduino I2C inspect: [`examples/arduino/i2c/commissioning/inspect_device/inspect_device.ino`](./examples/arduino/i2c/commissioning/inspect_device/inspect_device.ino)
- Arduino UART inspect: [`examples/arduino/uart/commissioning/inspect_device/inspect_device.ino`](./examples/arduino/uart/commissioning/inspect_device/inspect_device.ino)
- Arduino I2C typed D.O.: [`examples/arduino/i2c/typed/read_do/read_do.ino`](./examples/arduino/i2c/typed/read_do/read_do.ino)
- Arduino UART typed D.O.: [`examples/arduino/uart/typed/read_do/read_do.ino`](./examples/arduino/uart/typed/read_do/read_do.ino)
- Arduino I2C advanced RTD-driven EC temperature compensation:
  [`examples/arduino/i2c/advanced/ec_temp_comp_from_rtd/ec_temp_comp_from_rtd.ino`](./examples/arduino/i2c/advanced/ec_temp_comp_from_rtd/ec_temp_comp_from_rtd.ino)
- Arduino UART advanced multi-device routing:
  [`examples/arduino/uart/advanced/multi_device_router/multi_device_router.ino`](./examples/arduino/uart/advanced/multi_device_router/multi_device_router.ino)

## Entry Points

Primary public headers:

- [`src/ezo.h`](./src/ezo.h)
- [`src/ezo_do.h`](./src/ezo_do.h)
- [`src/ezo_ec.h`](./src/ezo_ec.h)
- [`src/ezo_hum.h`](./src/ezo_hum.h)
- [`src/ezo_calibration_transfer.h`](./src/ezo_calibration_transfer.h)
- [`src/ezo_control.h`](./src/ezo_control.h)
- [`src/ezo_parse.h`](./src/ezo_parse.h)
- [`src/ezo_i2c.h`](./src/ezo_i2c.h)
- [`src/ezo_i2c.hpp`](./src/ezo_i2c.hpp)
- [`src/ezo_i2c_arduino_wire.h`](./src/ezo_i2c_arduino_wire.h)
- [`src/ezo_i2c_linux_i2c.h`](./src/ezo_i2c_linux_i2c.h)
- [`src/ezo_orp.h`](./src/ezo_orp.h)
- [`src/ezo_ph.h`](./src/ezo_ph.h)
- [`src/ezo_product.h`](./src/ezo_product.h)
- [`src/ezo_rtd.h`](./src/ezo_rtd.h)
- [`src/ezo_schema.h`](./src/ezo_schema.h)
- [`src/ezo_uart.h`](./src/ezo_uart.h)
- [`src/ezo_uart_posix_serial.h`](./src/ezo_uart_posix_serial.h)
- [`src/ezo_uart_arduino_stream.h`](./src/ezo_uart_arduino_stream.h)

Primary implementation files:

- [`src/ezo.c`](./src/ezo.c)
- [`src/ezo_calibration_transfer.c`](./src/ezo_calibration_transfer.c)
- [`src/ezo_common.c`](./src/ezo_common.c)
- [`src/ezo_control.c`](./src/ezo_control.c)
- [`src/ezo_do.c`](./src/ezo_do.c)
- [`src/ezo_ec.c`](./src/ezo_ec.c)
- [`src/ezo_hum.c`](./src/ezo_hum.c)
- [`src/ezo_i2c.c`](./src/ezo_i2c.c)
- [`src/ezo_i2c_arduino_wire.cpp`](./src/ezo_i2c_arduino_wire.cpp)
- [`platform/linux/ezo_i2c_linux_i2c.c`](./platform/linux/ezo_i2c_linux_i2c.c)
- [`src/ezo_orp.c`](./src/ezo_orp.c)
- [`src/ezo_ph.c`](./src/ezo_ph.c)
- [`src/ezo_parse.c`](./src/ezo_parse.c)
- [`src/ezo_product.c`](./src/ezo_product.c)
- [`src/ezo_rtd.c`](./src/ezo_rtd.c)
- [`src/ezo_schema.c`](./src/ezo_schema.c)
- [`src/ezo_uart.c`](./src/ezo_uart.c)
- [`platform/linux/ezo_uart_posix_serial.c`](./platform/linux/ezo_uart_posix_serial.c)
- [`src/ezo_uart_arduino_stream.cpp`](./src/ezo_uart_arduino_stream.cpp)

## Docs

- [`docs/ezo/README.md`](./docs/ezo/README.md): curated EZO product and protocol context for this repo
- [`docs/public-api-layers.md`](./docs/public-api-layers.md): where to start by API layer and use case
- [`docs/support-matrix.md`](./docs/support-matrix.md): tracked public support statement and tier policy
- [`docs/product-onboarding.md`](./docs/product-onboarding.md): maintainer checklist for onboarding another product family
- [`docs/examples.md`](./docs/examples.md): staged example chooser and full matrix
- [`bindings/python/README.md`](./bindings/python/README.md): canonical Linux Python bindings doc
- [`docs/architecture.md`](./docs/architecture.md): structure, boundaries, packaging, validation
- [`docs/api-contract.md`](./docs/api-contract.md): behavioral contract for the public API
- [`docs/canonical-library-direction.md`](./docs/canonical-library-direction.md): stable long-term direction for the canonical product-aware library
- [`CHANGELOG.md`](./CHANGELOG.md): tracked change history

## Scope Notes

Intentionally out of scope for the current baseline:

- async/state-machine behavior
- hidden reconnect or resynchronization workflows around rebooting, sleep, or mode changes
- hidden retries or hidden delays
- UART C++ wrapper
