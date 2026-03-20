# Architecture

## Repo Shape

The repo is organized around one canonical library root: `src/`.

Inside `src/`:

- `ezo.h`, `ezo.c`: shared public results, timing hints, and numeric parsing
- `ezo_common.h`, `ezo_common.c`: internal shared formatting and parsing helpers
- `ezo_i2c.h`, `ezo_i2c.c`: I2C C core
- `ezo_i2c.hpp`: thin I2C C++ wrapper
- `ezo_i2c_arduino_wire.h`, `ezo_i2c_arduino_wire.cpp`: Arduino `TwoWire` I2C adapter
- `ezo_i2c_linux_i2c.h`: Linux I2C adapter public header
- `ezo_uart.h`, `ezo_uart.c`: UART C core
- `ezo_uart_arduino_stream.h`, `ezo_uart_arduino_stream.cpp`: Arduino `Stream` UART adapter

Host-only implementation code lives outside `src/`:

- `platform/linux/`: Linux I2C adapter implementation

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

4. UART core
   - line-oriented send flow
   - CR-terminated response assembly
   - response classification (`DATA`, `*OK`, `*ER`)

5. C++ wrapper
   - thin convenience layer over the I2C C API
   - no separate protocol logic

6. Platform integrations
   - convert platform APIs into the transport callback contracts
   - current integrations: Arduino `TwoWire`, Arduino `Stream`, and Linux file-descriptor I2C

## Transport Boundary

The cores talk to the outside world through injected transport callbacks plus caller-owned context.

The transport contracts are deliberately different by mode:

- I2C uses one addressed `write_then_read(...)` transaction callback
- UART uses `write_bytes(...)`, `read_bytes(...)`, and optional `discard_input(...)`

That separation is intentional. The repo does not pretend I2C and UART are the same transport model.

## API Direction

The public API is split into:

- shared public helpers in `src/ezo.h`
- I2C API in `src/ezo_i2c.h` and `src/ezo_i2c.hpp`
- UART API in `src/ezo_uart.h`
- Arduino integration headers in `src/ezo_i2c_arduino_wire.h` and `src/ezo_uart_arduino_stream.h`

Current surface:

- shared timing and numeric parsing helpers
- I2C device init, command send helpers, text reads, raw reads
- UART device init, command send helpers, line reads, discard hook
- Arduino I2C and UART adapter shims

Explicit non-goals for the current baseline:

- typed pH/EC/RTD helper APIs
- async/state-machine APIs
- hidden retries or hidden sleeps
- compatibility with the legacy Atlas API shape
- POSIX UART adapter
- UART C++ wrapper

## Validation

Validation is split across:

- host-side C and C++ tests against fake transports
- Linux I2C adapter behavior tests on host builds
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
- `docs/ezo/README.md` for product-family and protocol context
- `src/ezo.h` for shared types and helpers
- `src/ezo_i2c.h` for the I2C C API
- `src/ezo_i2c.hpp` for the I2C C++ wrapper
- `src/ezo_i2c_arduino_wire.h` for Arduino I2C integration
- `src/ezo_i2c_linux_i2c.h` for Linux I2C integration
- `src/ezo_uart.h` for the UART C API
- `src/ezo_uart_arduino_stream.h` for Arduino UART integration
- `src/ezo_common.c` for shared helper behavior
- `src/ezo_i2c.c` for I2C-specific core behavior
- `src/ezo_uart.c` for UART-specific core behavior
