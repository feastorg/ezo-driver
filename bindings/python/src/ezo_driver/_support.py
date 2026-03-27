from __future__ import annotations

from ._call import ascii_bytes, decode_cstr, ffi, lib
from .enums import CommandKind, DeviceStatus, UARTResponseKind
from .errors import raise_for_result


def _coerce_command_kind(kind: CommandKind | int) -> int:
    return int(kind)


class _I2CDeviceBase:
    def close(self) -> None:
        raise NotImplementedError

    @property
    def address(self) -> int:
        return int(lib.ezo_device_get_address(self._core))

    def set_address(self, address: int) -> None:
        lib.ezo_device_set_address(self._core, int(address))

    @property
    def last_status(self) -> int:
        return int(lib.ezo_device_get_last_status(self._core))

    def send_command(self, command: str, kind: CommandKind | int = CommandKind.GENERIC) -> int:
        payload = ascii_bytes(command)
        timing = ffi.new("ezo_timing_hint_t *")
        raise_for_result(
            int(lib.ezo_send_command(self._core, payload, _coerce_command_kind(kind), timing))
        )
        return int(timing.wait_ms)

    def send_command_with_float(
        self,
        prefix: str,
        value: float,
        decimals: int = 2,
        kind: CommandKind | int = CommandKind.GENERIC,
    ) -> int:
        timing = ffi.new("ezo_timing_hint_t *")
        raise_for_result(
            int(
                lib.ezo_send_command_with_float(
                    self._core,
                    ascii_bytes(prefix),
                    float(value),
                    int(decimals),
                    _coerce_command_kind(kind),
                    timing,
                )
            )
        )
        return int(timing.wait_ms)

    def send_read(self) -> int:
        timing = ffi.new("ezo_timing_hint_t *")
        raise_for_result(int(lib.ezo_send_read(self._core, timing)))
        return int(timing.wait_ms)

    def send_read_with_temp_comp(self, temperature_c: float, decimals: int = 2) -> int:
        timing = ffi.new("ezo_timing_hint_t *")
        raise_for_result(
            int(lib.ezo_send_read_with_temp_comp(self._core, float(temperature_c), int(decimals), timing))
        )
        return int(timing.wait_ms)

    def read_response_raw(self) -> tuple[int, bytes]:
        buffer = ffi.new(f"uint8_t[{int(lib.EZO_I2C_MAX_RESPONSE_PAYLOAD_LEN)}]")
        length = ffi.new("size_t *")
        status = ffi.new("ezo_device_status_t *")
        raise_for_result(
            int(
                lib.ezo_read_response_raw(
                    self._core,
                    buffer,
                    int(lib.EZO_I2C_MAX_RESPONSE_PAYLOAD_LEN),
                    length,
                    status,
                )
            )
        )
        return int(status[0]), bytes(ffi.buffer(buffer, int(length[0])))

    def read_response(self) -> tuple[int, str]:
        buffer = ffi.new(f"char[{int(lib.EZO_I2C_MAX_TEXT_RESPONSE_CAPACITY)}]")
        length = ffi.new("size_t *")
        status = ffi.new("ezo_device_status_t *")
        raise_for_result(
            int(
                lib.ezo_read_response(
                    self._core,
                    buffer,
                    int(lib.EZO_I2C_MAX_TEXT_RESPONSE_CAPACITY),
                    length,
                    status,
                )
            )
        )
        return int(status[0]), decode_cstr(buffer)


class _UARTDeviceBase:
    def close(self) -> None:
        raise NotImplementedError

    @property
    def last_response_kind(self) -> int:
        return int(lib.ezo_uart_device_get_last_response_kind(self._core))

    def send_command(self, command: str, kind: CommandKind | int = CommandKind.GENERIC) -> int:
        payload = ascii_bytes(command)
        timing = ffi.new("ezo_timing_hint_t *")
        raise_for_result(
            int(lib.ezo_uart_send_command(self._core, payload, _coerce_command_kind(kind), timing))
        )
        return int(timing.wait_ms)

    def send_command_with_float(
        self,
        prefix: str,
        value: float,
        decimals: int = 2,
        kind: CommandKind | int = CommandKind.GENERIC,
    ) -> int:
        timing = ffi.new("ezo_timing_hint_t *")
        raise_for_result(
            int(
                lib.ezo_uart_send_command_with_float(
                    self._core,
                    ascii_bytes(prefix),
                    float(value),
                    int(decimals),
                    _coerce_command_kind(kind),
                    timing,
                )
            )
        )
        return int(timing.wait_ms)

    def send_read(self) -> int:
        timing = ffi.new("ezo_timing_hint_t *")
        raise_for_result(int(lib.ezo_uart_send_read(self._core, timing)))
        return int(timing.wait_ms)

    def send_read_with_temp_comp(self, temperature_c: float, decimals: int = 2) -> int:
        timing = ffi.new("ezo_timing_hint_t *")
        raise_for_result(
            int(
                lib.ezo_uart_send_read_with_temp_comp(
                    self._core,
                    float(temperature_c),
                    int(decimals),
                    timing,
                )
            )
        )
        return int(timing.wait_ms)

    def read_line(self) -> tuple[int, str]:
        buffer = ffi.new(f"char[{int(lib.EZO_UART_MAX_TEXT_RESPONSE_CAPACITY)}]")
        length = ffi.new("size_t *")
        kind = ffi.new("ezo_uart_response_kind_t *")
        raise_for_result(
            int(
                lib.ezo_uart_read_line(
                    self._core,
                    buffer,
                    int(lib.EZO_UART_MAX_TEXT_RESPONSE_CAPACITY),
                    length,
                    kind,
                )
            )
        )
        return int(kind[0]), decode_cstr(buffer)

    def read_terminal_response(self) -> int:
        kind = ffi.new("ezo_uart_response_kind_t *")
        raise_for_result(int(lib.ezo_uart_read_terminal_response(self._core, kind)))
        return int(kind[0])

    def read_ok(self) -> None:
        raise_for_result(int(lib.ezo_uart_read_ok(self._core)))

    def discard_input(self) -> None:
        raise_for_result(int(lib.ezo_uart_discard_input(self._core)))


class _PublicI2CDeviceBase:
    _cdev: _I2CDeviceBase

    def close(self) -> None:
        self._cdev.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb) -> bool:
        self.close()
        return False

    @property
    def address(self) -> int:
        return self._cdev.address

    def set_address(self, address: int) -> None:
        self._cdev.set_address(address)

    @property
    def last_status(self) -> DeviceStatus:
        return DeviceStatus(self._cdev.last_status)

    def send_command(self, command: str, kind: CommandKind | int = CommandKind.GENERIC) -> int:
        return self._cdev.send_command(command, kind)

    def send_command_with_float(
        self,
        prefix: str,
        value: float,
        decimals: int = 2,
        kind: CommandKind | int = CommandKind.GENERIC,
    ) -> int:
        return self._cdev.send_command_with_float(prefix, value, decimals, kind)

    def send_read(self) -> int:
        return self._cdev.send_read()

    def send_read_with_temp_comp(self, temperature_c: float, decimals: int = 2) -> int:
        return self._cdev.send_read_with_temp_comp(temperature_c, decimals)

    def read_response_raw(self) -> tuple[DeviceStatus, bytes]:
        status, payload = self._cdev.read_response_raw()
        return DeviceStatus(status), payload

    def read_response(self) -> tuple[DeviceStatus, str]:
        status, payload = self._cdev.read_response()
        return DeviceStatus(status), payload


class _PublicUARTDeviceBase:
    _cdev: _UARTDeviceBase

    def close(self) -> None:
        self._cdev.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb) -> bool:
        self.close()
        return False

    @property
    def last_response_kind(self) -> UARTResponseKind:
        return UARTResponseKind(self._cdev.last_response_kind)

    def send_command(self, command: str, kind: CommandKind | int = CommandKind.GENERIC) -> int:
        return self._cdev.send_command(command, kind)

    def send_command_with_float(
        self,
        prefix: str,
        value: float,
        decimals: int = 2,
        kind: CommandKind | int = CommandKind.GENERIC,
    ) -> int:
        return self._cdev.send_command_with_float(prefix, value, decimals, kind)

    def send_read(self) -> int:
        return self._cdev.send_read()

    def send_read_with_temp_comp(self, temperature_c: float, decimals: int = 2) -> int:
        return self._cdev.send_read_with_temp_comp(temperature_c, decimals)

    def read_line(self) -> tuple[UARTResponseKind, str]:
        kind, payload = self._cdev.read_line()
        return UARTResponseKind(kind), payload

    def read_terminal_response(self) -> UARTResponseKind:
        return UARTResponseKind(self._cdev.read_terminal_response())

    def read_ok(self) -> None:
        self._cdev.read_ok()

    def discard_input(self) -> None:
        self._cdev.discard_input()


def _unwrap_i2c_device(device):
    if isinstance(device, _PublicI2CDeviceBase):
        return device._cdev._core
    if isinstance(device, _I2CDeviceBase):
        return device._core
    raise TypeError("expected an ezo_driver I2C device")


def _unwrap_uart_device(device):
    if isinstance(device, _PublicUARTDeviceBase):
        return device._cdev._core
    if isinstance(device, _UARTDeviceBase):
        return device._core
    raise TypeError("expected an ezo_driver UART device")


__all__ = [
    "_I2CDeviceBase",
    "_UARTDeviceBase",
    "_PublicI2CDeviceBase",
    "_PublicUARTDeviceBase",
    "_unwrap_i2c_device",
    "_unwrap_uart_device",
]
