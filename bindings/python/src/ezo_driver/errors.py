from __future__ import annotations

from ._ffi import ffi, lib
from .enums import DeviceStatus

_I2C_DEVICE_TYPE = ffi.typeof("ezo_i2c_device_t *")


class EzoError(Exception):
    """Base exception for Python binding failures."""


class EzoArgumentError(EzoError, ValueError):
    """Raised when the core library rejects invalid arguments."""


class EzoTransportError(EzoError):
    """Raised when a transport operation fails."""


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


def _raise_for_i2c_status(device, message: str) -> None:
    if not isinstance(device, ffi.CData) or ffi.typeof(device) is not _I2C_DEVICE_TYPE:
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
        raise EzoTransportError(message or "transport failure")
    if result == 4:
        _raise_for_i2c_status(device, message or "protocol failure")
        raise EzoProtocolError(message or "protocol failure")
    if result == 5:
        raise EzoParseError(message or "parse failure")
    raise EzoError(message or f"unknown ezo result: {result}")
