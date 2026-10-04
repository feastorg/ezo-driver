# Python Binding Reset Architecture

## Purpose

This document resets the Python binding direction around one simple rule:

The Python package must bind to the canonical C library. It must not become a second implementation of the library.

The current handwritten Python stack is too large and too manual for that goal. This document defines the proper replacement shape.

## Core Decision

The correct architecture is:

1. keep the protocol and product logic in the canonical C library
2. add a small host-facing C seam where the current C API is awkward for FFI
3. bind that C surface nearly 1:1 into Python
4. keep the Python layer thin and explicit

This is a reset away from a large handwritten Cython wrapper and toward a thin ABI bridge.

## Why The Current Shape Is Wrong

The current Python addition is too large because it duplicates work that should stay in C:

- it mirrors much of the C surface by hand
- it adds a second layer of manual Python data conversion
- it introduces wrapper modules that mostly forward to C
- it treats parity as a handwritten API design problem instead of a binding problem

That is the wrong maintenance model for this repo.

## What Stays Canonical

The canonical protocol and product API stays in `src/`.

That includes:

- `src/ezo.h`
- `src/ezo_i2c.h`
- `src/ezo_uart.h`
- `src/ezo_product.h`
- `src/ezo_parse.h`
- `src/ezo_schema.h`
- `src/ezo_control.h`
- `src/ezo_calibration_transfer.h`
- `src/ezo_ph.h`
- `src/ezo_orp.h`
- `src/ezo_rtd.h`
- `src/ezo_ec.h`
- `src/ezo_do.h`
- `src/ezo_hum.h`

The Python binding does not replace or reinterpret that surface. It maps it.

## What Actually Needs To Change In C

The core library does not need a redesign. It needs a narrow binding seam.

### 1. Add A Linux Host Device Facade

The current core transport model is good for embedded C but awkward for Python because the public seam is callback-based:

- `ezo_i2c_transport_t` in [src/ezo_i2c.h](/mnt/c/Users/Cameron/repos_feast/ezo-driver/src/ezo_i2c.h)
- `ezo_uart_transport_t` in [src/ezo_uart.h](/mnt/c/Users/Cameron/repos_feast/ezo-driver/src/ezo_uart.h)

Python should not need to understand or construct transport callback tables.

Add one new public Linux-only header:

- `src/ezo_linux_device.h`

This header should provide caller-owned Linux device wrappers that compose the existing core types with the existing Linux transport adapters.

Proposed shape:

```c
typedef struct {
  ezo_i2c_device_t core;
  ezo_linux_i2c_context_t context;
  int owns_fd;
} ezo_linux_i2c_device_t;

typedef struct {
  ezo_uart_device_t core;
  ezo_uart_posix_serial_t serial;
} ezo_linux_uart_device_t;

ezo_result_t ezo_linux_i2c_device_open_bus(ezo_linux_i2c_device_t *device,
                                           uint32_t bus_index,
                                           uint8_t address);
ezo_result_t ezo_linux_i2c_device_open_path(ezo_linux_i2c_device_t *device,
                                            const char *path,
                                            uint8_t address);
void ezo_linux_i2c_device_close(ezo_linux_i2c_device_t *device);
ezo_i2c_device_t *ezo_linux_i2c_device_core(ezo_linux_i2c_device_t *device);

ezo_result_t ezo_linux_uart_device_open(ezo_linux_uart_device_t *device,
                                        const char *path,
                                        ezo_uart_posix_baud_t baud,
                                        uint32_t read_timeout_ms);
void ezo_linux_uart_device_close(ezo_linux_uart_device_t *device);
ezo_uart_device_t *ezo_linux_uart_device_core(ezo_linux_uart_device_t *device);
```

Rules for this facade:

- caller-owned structs only
- no hidden allocation
- no Python-specific concepts
- no duplicate protocol logic
- no fake unified transport type

This facade exists to hide callback plumbing and Linux lifecycle details from the Python layer.

### 2. Tighten Existing C Contracts Before Binding Lock-In

The binding should not bake in rough or ambiguous C behavior.

Before the Python cutover, clean up the C surface where the current contracts are weak:

- fix the I2C text-response capacity mismatch around `EZO_I2C_MAX_TEXT_RESPONSE_LEN`
- make UART line reads recover cleanly from oversized responses
- reject numeric overflow in `ezo_parse_double`
- make response/status helper behavior explicit and symmetrical where needed

If a C contract is confusing to bind, fix the C contract first.

### 3. Keep The FFI Seam Host-Facing, Not Python-Specific

The new C seam is for any host binding, not just Python.

That means:

- no `PyObject *`
- no Python reference-count management in C
- no Python-specific naming
- no Python-specific error or container types

The C side exports plain functions, enums, structs, and buffers.

## Binding Tool Choice

Use `cffi` in API mode as the primary binding mechanism.

Why this fits this repo:

- it is designed to bind plain C APIs from declarations
- it can compile repo C sources together with the wrapper extension
- it does not require a giant handwritten wrapper layer
- it keeps the binding spec close to the C headers

This matches the documented `cffi` model for calling C sources directly, not only prebuilt shared libraries:

- https://cffi.readthedocs.io/en/stable/overview.html

Alternatives considered:

- `ctypes`: workable, but weaker for a repo this size with many structs, out-params, and bundled-source builds
- `SWIG`: powerful, but introduces a larger generator and typemap surface than needed right now
- large handwritten `Cython`: rejected because it already proved too manual and too large

## Python Layering

The Python package should have four layers only.

### 1. Build Spec Layer

This is the binding spec, not business logic.

Suggested files:

- `bindings/python/build_ffi.py`
- `bindings/python/cdef/ezo_base.h`
- `bindings/python/cdef/ezo_i2c.h`
- `bindings/python/cdef/ezo_uart.h`
- `bindings/python/cdef/ezo_product.h`
- `bindings/python/cdef/ezo_parse.h`
- `bindings/python/cdef/ezo_schema.h`
- `bindings/python/cdef/ezo_control.h`
- `bindings/python/cdef/ezo_calibration_transfer.h`
- `bindings/python/cdef/ezo_ph.h`
- `bindings/python/cdef/ezo_orp.h`
- `bindings/python/cdef/ezo_rtd.h`
- `bindings/python/cdef/ezo_ec.h`
- `bindings/python/cdef/ezo_do.h`
- `bindings/python/cdef/ezo_hum.h`
- `bindings/python/cdef/ezo_linux_device.h`

These files are curated declarations that stay very close to the C headers.

### 2. Runtime FFI Layer

Suggested file:

- `bindings/python/src/ezo_driver/_ffi.py`

Responsibilities:

- import the compiled `cffi` module
- expose `ffi` and `lib`
- define buffer-capacity helpers and small internal allocators

This layer contains no product logic.

### 3. Thin Python Runtime Helpers

Suggested files:

- `bindings/python/src/ezo_driver/errors.py`
- `bindings/python/src/ezo_driver/enums.py`
- `bindings/python/src/ezo_driver/types.py`
- `bindings/python/src/ezo_driver/_call.py`

Responsibilities:

- map `ezo_result_t` to Python exceptions
- expose `IntEnum` and `IntFlag` mirrors for stable public constants
- expose a small set of public result dataclasses for public POD structs
- provide generic helpers for the small number of call shapes used across the C API

The allowed call shapes are few:

- result only
- result plus timing hint
- result plus output struct
- result plus text buffer
- result plus raw buffer and explicit status or kind

This is where the binding stays thin. There must not be module-specific converter forests.

### 4. Public API Modules

Suggested files:

- `bindings/python/src/ezo_driver/__init__.py`
- `bindings/python/src/ezo_driver/i2c.py`
- `bindings/python/src/ezo_driver/uart.py`
- `bindings/python/src/ezo_driver/base.py`
- `bindings/python/src/ezo_driver/product.py`
- `bindings/python/src/ezo_driver/parse.py`
- `bindings/python/src/ezo_driver/schema.py`
- `bindings/python/src/ezo_driver/control.py`
- `bindings/python/src/ezo_driver/calibration_transfer.py`
- `bindings/python/src/ezo_driver/ph.py`
- `bindings/python/src/ezo_driver/orp.py`
- `bindings/python/src/ezo_driver/rtd.py`
- `bindings/python/src/ezo_driver/ec.py`
- `bindings/python/src/ezo_driver/do.py`
- `bindings/python/src/ezo_driver/hum.py`

Rules for the public layer:

- function names map directly from C with only the `ezo_` prefix removed
- transport suffixes such as `_i2c` and `_uart` stay explicit
- no unified device abstraction
- no hidden sleeps
- no hidden retries
- no hidden reconnect or resync behavior

The only stateful Python objects should be:

- `LinuxI2CDevice`
- `LinuxUARTDevice`
- `UARTSequence`, if exposed as a Python state helper over `ezo_uart_sequence_t`

Everything else stays function-oriented.

## Python Data Model

The Python layer should be thin, but it should not expose raw buffer bookkeeping.

### Devices

`LinuxI2CDevice` and `LinuxUARTDevice` are small owner classes around the new Linux C facade structs.

They handle:

- open
- close
- context manager support
- access to the underlying core pointer for internal wrapper calls

They do not implement protocol behavior.

### Enums And Flags

Public C enums become Python `IntEnum`.

Public C bitmasks become Python `IntFlag`.

These are stable, small, and worth mirroring explicitly.

### Struct Results

Public POD result structs become small frozen dataclasses with matching field names.

Examples:

- `DeviceInfo`
- `ControlStatus`
- `PhReading`
- `EcReading`
- `CalibrationExportInfo`

Rules:

- one Python type per public C POD shape
- field names follow the C struct
- no secondary dict conversion layer

### Strings And Bytes

- text commands and text responses use Python `str`
- raw response payloads and calibration chunks use Python `bytes`
- Python callers never pass output buffer lengths directly

### Errors

- any non-`EZO_OK` library result becomes a Python exception
- explicit I2C device status values remain return data
- explicit UART response kinds remain return data

Protocol statuses are not exceptions unless the C API itself returns an error.

## Packaging Shape

The package identity remains:

- distribution: `ezo-driver`
- import package: `ezo_driver`

Install from the repo root:

```bash
python -m pip install .
```

The package builds against the canonical repo sources:

- `src/`
- `platform/linux/`
- the new Linux host facade implementation

The package must not vendor copied C sources under `bindings/python/csrc/`.

## Test Shape

The test strategy should mirror the C library shape instead of inventing a second Python abstraction.

Required layers:

1. smoke import and editable install
2. raw Linux device lifecycle tests
3. shared helper parity tests
4. product module parity tests
5. regression tests for known C contract edge cases

For unit coverage, keep using the fake transports already maintained in `tests/fakes/`.

The Python tests should reuse that C fake infrastructure through a tiny test-only bridge instead of rewriting transport behavior in Python.

## Explicit Non-Goals

This reset does not aim to create:

- a Python-first redesign of the API
- a universal cross-platform Python transport abstraction
- implicit workflow automation
- a parallel implementation of parsing or protocol logic in Python
- a large code generator framework unless the thin binding still proves too repetitive

## End State

The end state is a Python package that is:

- full-parity with the supported C surface relevant to Linux use
- small enough to audit
- obviously bound to the C library instead of reimplemented in Python
- suitable for editable install now
- structurally ready for future PyPI packaging
