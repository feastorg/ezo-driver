# Architecture

## Repo Shape

The repo is organized around one canonical library root: `src/`.

Inside `src/`:

- `ezo.h`, `ezo.c`: shared public results, timing hints, and numeric parsing
- `ezo_common.h`, `ezo_common.c`: internal shared formatting and parsing helpers
- `ezo_do.h`, `ezo_do.c`: typed DO product module
- `ezo_ec.h`, `ezo_ec.c`: typed EC product module
- `ezo_hum.h`, `ezo_hum.c`: typed HUM product module
- `ezo_i2c.h`, `ezo_i2c.c`: I2C C core
- `ezo_i2c.hpp`: thin I2C C++ wrapper
- `ezo_i2c_arduino_wire.h`, `ezo_i2c_arduino_wire.cpp`: Arduino `TwoWire` I2C adapter
- `ezo_i2c_linux_i2c.h`: Linux I2C adapter public header
- `ezo_orp.h`, `ezo_orp.c`: typed ORP product module
- `ezo_parse.h`, `ezo_parse.c`: shared query, CSV, and UART-sequence helpers
- `ezo_ph.h`, `ezo_ph.c`: typed pH product module
- `ezo_product.h`, `ezo_product.c`: product identity and metadata layer
- `ezo_rtd.h`, `ezo_rtd.c`: typed RTD product module
- `ezo_schema.h`, `ezo_schema.c`: canonical output schemas and typed reading structs
- `ezo_uart.h`, `ezo_uart.c`: UART C core
- `ezo_uart_posix_serial.h`: POSIX UART adapter public header
- `ezo_uart_arduino_stream.h`, `ezo_uart_arduino_stream.cpp`: Arduino `Stream` UART adapter

Host-only implementation code lives outside `src/`:

- `platform/linux/`: Linux host adapter implementations

Everything else is supporting material:

- `platform/`: non-Arduino platform code
- `examples/`: small integration examples
- `tests/`: host-side tests and fakes
- `docs/`: tracked implementation docs and curated EZO product/protocol notes
- `_reference/`: legacy reference only

## Rules

1. The core stays C99 and platform-agnostic.
2. The C++ layer stays thin and convenience-only.
3. Platform transport code stays out of the core implementation.
4. The core does not allocate dynamically.
5. The core does not sleep internally.
6. Callers own buffers and timing behavior.
7. I2C and UART stay separate driver families.

## Layers

1. Shared public layer
   - result enum
   - command kinds
   - timing hints
   - numeric parsing helper

2. Shared internal helper layer
   - fixed-point command formatting
   - transport-neutral helper code

3. I2C core
   - addressed transaction send flow
   - status-byte response decoding
   - text and raw response handling

4. Product metadata layer
   - static registry for known products
   - device-info parsing and normalized identification
   - support tiers, capabilities, defaults, timing lookup, and timing fallback

5. UART core
   - line-oriented send flow
   - CR-terminated single-line read primitive
   - response classification and small sequence-oriented helpers

6. Shared product-facing parse/schema layer
   - borrowed text spans and CSV/query parsing
   - canonical field-order descriptors per product
   - typed scalar and multi-output reading structs
   - transport-neutral sequence state for higher-level UART workflows

7. Typed product modules
   - pH read, temperature, calibration-status, and slope helpers
   - ORP read, calibration, and extended-scale helpers
   - RTD read, scale, and calibration-status helpers
   - EC multi-output read, output-config, temperature, probe-K, and TDS-factor helpers
   - DO multi-output read, output-config, temperature, salinity, and pressure helpers
   - HUM multi-output read and output-config helpers
   - explicit I2C and UART entry points with no fake unified device

8. C++ wrapper
   - thin convenience layer over the I2C C API
   - no separate protocol logic

9. Platform integrations
   - convert platform APIs into the transport callback contracts
   - current integrations: Arduino `TwoWire`, Arduino `Stream`, Linux file-descriptor I2C, and Linux host POSIX serial

## Transport Boundary

The cores talk to the outside world through injected transport callbacks plus caller-owned context.

The transport contracts are deliberately different by mode:

- I2C uses one addressed `write_then_read(...)` transaction callback
- UART uses `write_bytes(...)`, `read_bytes(...)`, and optional `discard_input(...)`

That separation is intentional. The repo does not pretend I2C and UART are the same transport model.

## API Direction

The public API is split into:

- shared public helpers in `src/ezo.h`
- multi-output product APIs in `src/ezo_ec.h`, `src/ezo_do.h`, and `src/ezo_hum.h`
- shared parse helpers in `src/ezo_parse.h`
- I2C API in `src/ezo_i2c.h` and `src/ezo_i2c.hpp`
- scalar product APIs in `src/ezo_ph.h`, `src/ezo_orp.h`, and `src/ezo_rtd.h`
- product metadata API in `src/ezo_product.h`
- shared schema API in `src/ezo_schema.h`
- UART API in `src/ezo_uart.h`
- POSIX UART adapter API in `src/ezo_uart_posix_serial.h`
- Arduino integration headers in `src/ezo_i2c_arduino_wire.h` and `src/ezo_uart_arduino_stream.h`

Current surface:

- shared timing and numeric parsing helpers
- shared query parsing, sequence-state, and schema helpers
- I2C device init, command send helpers, text reads, raw reads
- product IDs, metadata lookup, device-info parsing, timing lookup, timing fallback
- typed read/query helpers over both transports for pH, ORP, RTD, EC, DO, and HUM
- UART device init, command send helpers, line reads, discard hook
- Arduino I2C and UART adapter shims
- Linux I2C and Linux host POSIX UART adapters

Explicit non-goals for the current baseline:

- broad typed control-plane workflows
- advanced per-product helpers such as calibration transfer and HUM temperature calibration
- async/state-machine APIs
- hidden retries or hidden sleeps
- compatibility with the legacy Atlas API shape
- UART C++ wrapper

## Validation

Validation is split across:

- host-side C and C++ tests against fake transports
- Linux I2C and Linux host POSIX UART adapter behavior tests on host builds
- PlatformIO Arduino compile coverage for both I2C and UART examples
- manual Arduino IDE validation

## Packaging

Primary development flow:

- CMake for host builds and tests

Packaging/distribution surfaces:

- `library.properties` for Arduino tooling
- `library.json` for PlatformIO

Arduino packaging now covers both the I2C and UART Arduino-facing headers. Host-only Linux support remains a CMake-side concern.

## Handoff Notes

A new developer should treat these files as the main entry points:

- `README.md` for first-contact orientation
- `docs/canonical-library-direction.md` for the long-term product-aware direction beyond the current transport baseline
- `docs/ezo/README.md` for product-family and protocol context
- `src/ezo.h` for shared types and helpers
- `src/ezo_do.h` for typed DO helpers
- `src/ezo_ec.h` for typed EC helpers
- `src/ezo_hum.h` for typed HUM helpers
- `src/ezo_parse.h` for shared text, query, and sequence helpers
- `src/ezo_i2c.h` for the I2C C API
- `src/ezo_i2c.hpp` for the I2C C++ wrapper
- `src/ezo_i2c_arduino_wire.h` for Arduino I2C integration
- `src/ezo_i2c_linux_i2c.h` for Linux I2C integration
- `src/ezo_orp.h` for typed ORP helpers
- `src/ezo_ph.h` for typed pH helpers
- `src/ezo_product.h` for product IDs, metadata, and device-info parsing
- `src/ezo_rtd.h` for typed RTD helpers
- `src/ezo_schema.h` for canonical field-order and typed reading helpers
- `src/ezo_uart.h` for the UART C API
- `src/ezo_uart_posix_serial.h` for POSIX UART integration
- `src/ezo_uart_arduino_stream.h` for Arduino UART integration
- `src/ezo_common.c` for shared helper behavior
- `src/ezo_do.c` for typed DO helper behavior
- `src/ezo_ec.c` for typed EC helper behavior
- `src/ezo_hum.c` for typed HUM helper behavior
- `src/ezo_i2c.c` for I2C-specific core behavior
- `src/ezo_orp.c` for typed ORP helper behavior
- `src/ezo_ph.c` for typed pH helper behavior
- `src/ezo_parse.c` for shared query/CSV parsing and UART sequence state
- `src/ezo_product.c` for the static product registry and lookup logic
- `src/ezo_rtd.c` for typed RTD helper behavior
- `src/ezo_schema.c` for canonical product schemas and typed reading helpers
- `src/ezo_uart.c` for UART-specific core behavior
- `platform/linux/ezo_uart_posix_serial.c` for POSIX UART adapter behavior
