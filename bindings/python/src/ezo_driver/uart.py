from __future__ import annotations

from ._ffi import ffi, lib
from ._support import _PublicUARTDeviceBase, _UARTDeviceBase
from .errors import raise_for_result

MAX_TEXT_RESPONSE_LEN = int(lib.EZO_UART_MAX_TEXT_RESPONSE_LEN)
MAX_TEXT_RESPONSE_CAPACITY = int(lib.EZO_UART_MAX_TEXT_RESPONSE_CAPACITY)


class _LinuxUARTDevice(_UARTDeviceBase):
    def __init__(self, path: str, baud: int = 9600, read_timeout_ms: int = 1000):
        if not path or int(read_timeout_ms) <= 0:
            raise ValueError("invalid Linux UART device arguments")
        self._device = ffi.new("ezo_linux_uart_device_t *")
        raise_for_result(
            int(
                lib.ezo_linux_uart_device_open(
                    self._device,
                    path.encode("ascii"),
                    int(baud),
                    int(read_timeout_ms),
                )
            )
        )
        self._core = lib.ezo_linux_uart_device_core(self._device)

    def close(self) -> None:
        if getattr(self, "_device", None) is not None:
            lib.ezo_linux_uart_device_close(self._device)


class LinuxUARTDevice(_PublicUARTDeviceBase):
    def __init__(self, path: str, baud: int = 9600, read_timeout_ms: int = 1000):
        self._cdev = _LinuxUARTDevice(path, baud, read_timeout_ms)


def response_kind_is_control(kind: int) -> bool:
    return bool(lib.ezo_uart_response_kind_is_control(int(kind)))


def response_kind_is_terminal(kind: int) -> bool:
    return bool(lib.ezo_uart_response_kind_is_terminal(int(kind)))


__all__ = [
    "LinuxUARTDevice",
    "MAX_TEXT_RESPONSE_LEN",
    "MAX_TEXT_RESPONSE_CAPACITY",
    "response_kind_is_control",
    "response_kind_is_terminal",
]
