from __future__ import annotations

from ._ffi import ffi, lib
from ._support import _I2CDeviceBase, _PublicI2CDeviceBase
from .errors import raise_for_result

MAX_RESPONSE_PAYLOAD_LEN = int(lib.EZO_I2C_MAX_RESPONSE_PAYLOAD_LEN)
MAX_TEXT_RESPONSE_LEN = int(lib.EZO_I2C_MAX_TEXT_RESPONSE_LEN)
MAX_TEXT_RESPONSE_CAPACITY = int(lib.EZO_I2C_MAX_TEXT_RESPONSE_CAPACITY)


class _LinuxI2CDevice(_I2CDeviceBase):
    def __init__(self, bus: int, address: int):
        if int(bus) < 0 or not 0 <= int(address) <= 0x7F:
            raise ValueError("invalid Linux I2C device arguments")
        self._device = ffi.new("ezo_linux_i2c_device_t *")
        raise_for_result(int(lib.ezo_linux_i2c_device_open_bus(self._device, int(bus), int(address))))
        self._core = lib.ezo_linux_i2c_device_core(self._device)

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
