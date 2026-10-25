from __future__ import annotations

import weakref

from ._ffi import ffi, lib
from .enums import DeviceStatus

_I2C_DEVICE_TYPE = ffi.typeof("ezo_i2c_device_t *")

# Open Linux I2C devices by the address of their core, so a transport failure
# reported against the core can find the bus and the context's last_errno. An
# entry goes when its device is garbage-collected, which also frees the core.
_linux_i2c_devices: weakref.WeakValueDictionary[int, object] = weakref.WeakValueDictionary()


class EzoError(Exception):
    """Base exception for Python binding failures."""


class EzoArgumentError(EzoError, ValueError):
    """Raised when the core library rejects invalid arguments."""


class EzoTransportError(EzoError):
    """Raised when a transport operation fails.

    ``errno`` is the OS error the Linux I2C transport recorded, ``bus`` the
    ``/dev/i2c-N`` bus number and ``address`` the I2C address; each is
    ``None`` where unknown (fake and UART transports, for example).
    """

    def __init__(
        self,
        *args: object,
        errno: int | None = None,
        bus: int | None = None,
        address: int | None = None,
    ):
        super().__init__(*args)
        self.errno = errno
        self.bus = bus
        self.address = address


class EzoProtocolError(EzoError):
    """Raised when a device response violates the expected protocol."""


class EzoNotReadyError(EzoProtocolError):
    """Raised when an I2C read finds the circuit still processing (status NOT_READY)."""


class EzoDeviceStatusError(EzoProtocolError):
    """Raised when an I2C read gets status FAIL or NO_DATA; ``status`` holds which."""

    def __init__(self, message: str, status: DeviceStatus):
        super().__init__(message)
        self.status = status


class EzoParseError(EzoError):
    """Raised when a response payload cannot be parsed."""


def _is_i2c_core(device) -> bool:
    return isinstance(device, ffi.CData) and ffi.typeof(device) is _I2C_DEVICE_TYPE


def _register_linux_i2c(device) -> None:
    """Record an open Linux I2C device; it needs ``_core``, ``_device`` and ``_bus``."""
    _linux_i2c_devices[int(ffi.cast("uintptr_t", device._core))] = device


def _transport_error(device, message: str) -> EzoTransportError:
    if not _is_i2c_core(device):
        return EzoTransportError(message)
    address = int(lib.ezo_device_get_address(device))
    linux = _linux_i2c_devices.get(int(ffi.cast("uintptr_t", device)))
    if linux is None:
        return EzoTransportError(message, address=address)
    last_errno = int(linux._device.context.last_errno)
    return EzoTransportError(message, errno=last_errno or None, bus=linux._bus, address=address)


def _raise_for_i2c_status(device, message: str) -> None:
    if not _is_i2c_core(device):
        return
    status = DeviceStatus(int(lib.ezo_device_get_last_status(device)))
    if status == DeviceStatus.NOT_READY:
        raise EzoNotReadyError(message)
    if status in (DeviceStatus.FAIL, DeviceStatus.NO_DATA):
        raise EzoDeviceStatusError(message, status)


def raise_for_result(result: int, *, message: str | None = None, device=None) -> None:
    if result == 0:
        return
    if result == 1:
        raise EzoArgumentError(message or "invalid argument")
    if result == 2:
        raise EzoArgumentError(message or "buffer too small")
    if result == 3:
        raise _transport_error(device, message or "transport failure")
    if result == 4:
        _raise_for_i2c_status(device, message or "protocol failure")
        raise EzoProtocolError(message or "protocol failure")
    if result == 5:
        raise EzoParseError(message or "parse failure")
    raise EzoError(message or f"unknown ezo result: {result}")
