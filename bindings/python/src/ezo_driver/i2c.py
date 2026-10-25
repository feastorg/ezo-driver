from __future__ import annotations

from ._ffi import ffi, lib
from ._support import _I2CDeviceBase, _PublicI2CDeviceBase
from .errors import EzoTransportError, _register_linux_i2c, raise_for_result

MAX_RESPONSE_PAYLOAD_LEN = int(lib.EZO_I2C_MAX_RESPONSE_PAYLOAD_LEN)
MAX_TEXT_RESPONSE_LEN = int(lib.EZO_I2C_MAX_TEXT_RESPONSE_LEN)
MAX_TEXT_RESPONSE_CAPACITY = int(lib.EZO_I2C_MAX_TEXT_RESPONSE_CAPACITY)


class _LinuxI2CDevice(_I2CDeviceBase):
    def __init__(self, bus: int, address: int):
        if int(bus) < 0 or not 0 <= int(address) <= 0x7F:
            raise ValueError("invalid Linux I2C device arguments")
        self._bus = int(bus)
        self._device = ffi.new("ezo_linux_i2c_device_t *")
        result = int(lib.ezo_linux_i2c_device_open_bus(self._device, self._bus, int(address)))
        if result == 3:
            # The core is not initialised when open() fails, so the address
            # comes from the argument rather than ezo_device_get_address.
            raise EzoTransportError(
                "transport failure",
                errno=int(self._device.context.last_errno) or None,
                bus=self._bus,
                address=int(address),
            )
        raise_for_result(result)
        self._core = lib.ezo_linux_i2c_device_core(self._device)
        _register_linux_i2c(self)

    def close(self) -> None:
        if getattr(self, "_device", None) is not None:
            lib.ezo_linux_i2c_device_close(self._device)


class LinuxI2CDevice(_PublicI2CDeviceBase):
    def __init__(self, bus: int, address: int):
        self._cdev = _LinuxI2CDevice(bus, address)


def device_status_name(status: int) -> str:
    return ffi.string(lib.ezo_device_status_name(int(status))).decode("ascii")


__all__ = [
    "LinuxI2CDevice",
    "MAX_RESPONSE_PAYLOAD_LEN",
    "MAX_TEXT_RESPONSE_LEN",
    "MAX_TEXT_RESPONSE_CAPACITY",
    "device_status_name",
]
