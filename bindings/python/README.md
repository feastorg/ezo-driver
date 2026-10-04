# `ezo-driver` Python bindings

Linux-only Python bindings for the canonical `ezo-driver` C library.

## Support statement

- Supported host platform: Linux
- Supported transports: Linux I2C and POSIX UART
- Supported Python install mode for this stage: install from the repo checkout or a built sdist or wheel
- Public package name: `ezo-driver`
- Public import package: `ezo_driver`

## Install

From the repo root:

```bash
python -m pip install .
```

For development, install it editable with the test dependencies:

```bash
python -m pip install -e ".[test]"
python -m pytest bindings/python/tests
```

The packaging lives at the repo root (`pyproject.toml`, `setup.py`, `MANIFEST.in`) so the sdist carries the C sources from `src/` and `platform/linux/` and a wheel can be built from it.

The shipped `ezo_driver._native` module holds the C core and the Linux transports only. The tests drive it through the fake transports from `tests/fakes/`, which `bindings/python/tests/_fakes.py` compiles into a separate test-only cffi module in a temporary directory each session; this needs a C compiler and the `test` extra. The C sources are found in the checkout that holds the tests; when the tests run from a copy elsewhere, point `EZO_DRIVER_SOURCE_DIR` at the checkout.

## Public modules

- `ezo_driver.errors`
- `ezo_driver.enums`
- `ezo_driver.types`
- `ezo_driver.base`
- `ezo_driver.i2c`
- `ezo_driver.uart`
- `ezo_driver.product`
- `ezo_driver.parse`
- `ezo_driver.schema`
- `ezo_driver.control`
- `ezo_driver.calibration_transfer`
- `ezo_driver.ph`
- `ezo_driver.orp`
- `ezo_driver.rtd`
- `ezo_driver.ec`
- `ezo_driver.do`
- `ezo_driver.hum`

Top-level `ezo_driver` exports only the stable transport constructors, exception hierarchy, and shared enums/types.

## I2C example

```python
from ezo_driver import LinuxI2CDevice, ProductId
from ezo_driver import control, ph

with LinuxI2CDevice(bus=1, address=0x63) as dev:
    wait_ms = control.send_info_query_i2c(dev, ProductId.PH)
    info = control.read_info_i2c(dev)

    wait_ms = ph.send_read_i2c(dev)
    reading = ph.read_response_i2c(dev)

print(info)
print(reading)
```

## UART example

```python
from ezo_driver import LinuxUARTDevice, ProductId
from ezo_driver import control, ph

with LinuxUARTDevice("/dev/ttyUSB0", baud=9600, read_timeout_ms=1000) as dev:
    wait_ms = control.send_info_query_uart(dev, ProductId.PH)
    info = control.read_info_uart(dev)

    wait_ms = ph.send_read_uart(dev)
    reading = ph.read_response_uart(dev)

print(info)
print(reading)
```

## Design constraints

- The Python layer stays transport-explicit. There is no unified device abstraction.
- The bindings mirror the C library concepts instead of exposing buffer management.
- Timing hints are returned as integer milliseconds.
- I2C device status codes and UART response kinds stay explicit values in the API.

## Non-goals

- No implicit sleeps
- No implicit retries
- No implicit reconnect/resynchronization
- No workflow wrapper above the canonical transport and product helpers
