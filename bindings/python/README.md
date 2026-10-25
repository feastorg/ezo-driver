# `ezo-driver` Python bindings

Linux-only Python bindings for the canonical `ezo-driver` C library.

## Support statement

- Supported host platform: Linux
- Supported transports: Linux I2C and POSIX UART
- Supported Python install mode: `pip install ezo-driver` from PyPI, with wheels for x86_64 and aarch64 Linux on Python 3.9+; other platforms build from the sdist or the repo checkout
- Public package name: `ezo-driver`
- Public import package: `ezo_driver`

## Install

```bash
python -m pip install ezo-driver
```

PyPI carries `cp39-abi3` manylinux wheels for x86_64 and aarch64 Linux, so any CPython 3.9 or newer installs one without a compiler, a 64-bit Raspberry Pi included. On other platforms pip builds from the sdist, which needs a C compiler and the Python headers.

From the repo root:

```bash
python -m pip install .
```

For development, install it editable with the test dependencies:

```bash
python -m pip install -e ".[test]"
python -m pytest bindings/python/tests
```

The packaging lives at the repo root (`pyproject.toml`, `setup.py`, `setup.cfg`, `MANIFEST.in`) so the sdist carries the C sources from `src/` and `platform/linux/` and a wheel can be built from it. `setup.cfg` tags the wheel `cp39-abi3`: cffi builds `ezo_driver._native` against the limited C API, so one wheel per architecture covers every supported Python. The release workflow builds the wheels with cibuildwheel (`[tool.cibuildwheel]` in `pyproject.toml`) and publishes them to PyPI.

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

## Split-phase reads

Every exchange with a circuit has two halves. A `send_*` helper writes the command and returns how long the circuit needs, in milliseconds. The matching `read_*` helper fetches the reply. The bindings never sleep, so the caller waits in between, and the wait is the caller's to schedule: block a thread, or do other work and come back.

On I2C, a read that comes too early raises `EzoNotReadyError` with the device's `last_status` at `DeviceStatus.NOT_READY`: the circuit is still working, so wait and read again. A read that gets `DeviceStatus.FAIL` or `DeviceStatus.NO_DATA` raises `EzoDeviceStatusError`, whose `status` says which. Both subclass `EzoProtocolError`, which still catches them; UART reads and other protocol failures raise `EzoProtocolError` itself.

A transport failure raises `EzoTransportError` with three attributes, each `None` where unknown. `errno` is the OS error the Linux I2C transport recorded: for example `ENOENT` when `LinuxI2CDevice(bus, address)` names a bus with no `/dev/i2c-N`, or `EIO` for a short write. `bus` is the bus number given to `LinuxI2CDevice`, and `address` the device's I2C address. A fake I2C transport sets only `address`, and UART sets none of them. The message stays `transport failure`.

## I2C example

```python
import time

from ezo_driver import LinuxI2CDevice, ProductId
from ezo_driver import control, ph

with LinuxI2CDevice(bus=1, address=0x63) as dev:
    wait_ms = control.send_info_query_i2c(dev, ProductId.PH)
    time.sleep(wait_ms / 1000)
    info = control.read_info_i2c(dev)

    wait_ms = ph.send_read_i2c(dev)
    time.sleep(wait_ms / 1000)
    reading = ph.read_response_i2c(dev)

print(info)
print(reading)
```

## UART example

```python
import time

from ezo_driver import LinuxUARTDevice, ProductId
from ezo_driver import control, ph

with LinuxUARTDevice("/dev/ttyUSB0", baud=9600, read_timeout_ms=1000) as dev:
    wait_ms = control.send_info_query_uart(dev, ProductId.PH)
    time.sleep(wait_ms / 1000)
    info = control.read_info_uart(dev)

    wait_ms = ph.send_read_uart(dev)
    time.sleep(wait_ms / 1000)
    reading = ph.read_response_uart(dev)

print(info)
print(reading)
```

## Caching output masks

EC, DO and HUM circuits report only the outputs that are switched on, so `ec.read_response_*`, `do.read_response_*` and `hum.read_response_*` take the enabled-output mask that tells them how to split the reply. Query it once with `send_output_query_*` and `read_output_config_*`, keep `enabled_mask`, and pass it to every read. The mask changes only when an output is switched with `send_output_set_*`, so query it again after that, or after anything else that may have reconfigured the circuit (a factory reset, or another program).

```python
import time

from ezo_driver import LinuxI2CDevice
from ezo_driver import ec
from ezo_driver.enums import ECOutputMask

with LinuxI2CDevice(bus=1, address=0x64) as dev:
    time.sleep(ec.send_output_query_i2c(dev) / 1000)
    mask = ec.read_output_config_i2c(dev).enabled_mask

    for _ in range(3):
        time.sleep(ec.send_read_i2c(dev) / 1000)
        reading = ec.read_response_i2c(dev, mask)
        if ECOutputMask.CONDUCTIVITY in reading.present_mask:
            print(reading.conductivity_us_cm)
```

## Sharing a bus between threads

The bindings take no locks. The rules follow from the transport:

- A circuit holds the reply to its last command only, so the whole send, wait, read cycle for one circuit must run without another command to that circuit in between.
- A device object is not safe for concurrent use. Use each `LinuxI2CDevice` or `LinuxUARTDevice` from one thread, or guard it with a lock held across the whole cycle. `set_address()` changes the address for every later call on that object.
- Each `LinuxI2CDevice` opens its own file descriptor on `/dev/i2c-<bus>` and sets the address before each transfer, so separate device objects on one bus can run in separate threads; the kernel serialises the transfers on the bus. Open one object per circuit rather than sharing one and switching its address.
- A UART port carries one circuit. Open it once.

```python
import threading
import time

from ezo_driver import LinuxI2CDevice
from ezo_driver import ec, ph

results = {}


def poll_ph():
    with LinuxI2CDevice(bus=1, address=0x63) as dev:
        time.sleep(ph.send_read_i2c(dev) / 1000)
        results["ph"] = ph.read_response_i2c(dev).ph


def poll_ec():
    with LinuxI2CDevice(bus=1, address=0x64) as dev:
        time.sleep(ec.send_output_query_i2c(dev) / 1000)
        mask = ec.read_output_config_i2c(dev).enabled_mask
        time.sleep(ec.send_read_i2c(dev) / 1000)
        results["ec"] = ec.read_response_i2c(dev, mask).conductivity_us_cm


threads = [threading.Thread(target=poll_ph), threading.Thread(target=poll_ec)]
for thread in threads:
    thread.start()
for thread in threads:
    thread.join()
print(results)
```

To share one device object between threads instead, hold one `threading.Lock` per circuit around each cycle:

```python
import threading
import time

from ezo_driver import LinuxI2CDevice
from ezo_driver import ph

dev = LinuxI2CDevice(bus=1, address=0x63)
dev_lock = threading.Lock()


def read_ph():
    with dev_lock:
        time.sleep(ph.send_read_i2c(dev) / 1000)
        return ph.read_response_i2c(dev).ph


try:
    print(read_ph())
finally:
    dev.close()
```

The tests run every Python example in this README against the fake transports (`bindings/python/tests/test_readme_examples.py`).

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
