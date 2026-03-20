# Architecture

## Repo Shape

The repo is organized around one canonical library root: `src/`.

Inside `src/`:

- `ezo_i2c.c`: core C99 implementation
- `ezo_i2c.h`, `ezo_i2c.hpp`: public core headers
- `ezo_i2c_arduino_wire.h`: Arduino sketch-facing transport header
- `ezo_i2c_linux_i2c.h`: host-side Linux transport header
- `ezo_i2c_arduino_wire.cpp`: Arduino `TwoWire` adapter implementation

Host-only implementation code lives outside `src/`:

- `platform/linux/`: Linux adapter implementation

Everything else is supporting material:

- `platform/`: non-Arduino platform code
- `examples/`: small integration examples
- `tests/`: host-side tests and fakes
- `docs/`: tracked implementation docs
- `_reference/`: legacy reference only

## Rules

1. The core stays C99 and platform-agnostic.
2. The C++ layer stays thin and header-only.
3. Platform transport code stays out of the core implementation.
4. The core does not allocate dynamically.
5. The core does not sleep internally.
6. Callers own buffers and timing behavior.

## Layers

1. Core
   - protocol formatting
   - response decoding
   - numeric helpers
   - timing hints

2. C++ wrapper
   - header-only convenience layer over the C core
   - no separate protocol logic

3. Platform integrations
   - convert platform I2C APIs into the transport callback contract
   - current integrations: Arduino `TwoWire`, Linux file descriptor transport

## Transport Boundary

The core talks to the outside world through a single injected transport callback plus caller-owned context.

That boundary is the main portability seam.

## API Direction

The public API is intentionally generic.

Current surface:

- device init and address management
- generic command send helpers
- timing hints by command class
- text response reads
- raw response reads
- numeric parse helpers

Explicit non-goals for v1:

- typed pH/EC/RTD helper APIs
- async/state-machine APIs
- hidden retries or hidden sleeps
- compatibility with the legacy Atlas API shape

## Validation

Validation is split across:

- host-side C and C++ tests against fake transports
- Linux adapter behavior tests on host builds
- PlatformIO Arduino compile coverage
- manual Arduino IDE validation

## Packaging

Primary development flow:

- CMake for host builds and tests

Packaging/distribution surfaces:

- `library.properties` for Arduino tooling
- `library.json` for PlatformIO

## Handoff Notes

A new developer should treat these files as the main entry points:

- `README.md` for first-contact orientation
- `src/ezo_i2c.h` for the C API
- `src/ezo_i2c.hpp` for the C++ wrapper
- `src/ezo_i2c_arduino_wire.h` for Arduino sketch-facing integration
- `src/ezo_i2c_linux_i2c.h` for Linux-side transport integration
- `src/ezo_i2c.c` for core behavior
