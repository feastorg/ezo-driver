from __future__ import annotations

from ezo_driver._ffi import ffi, lib
from ezo_driver._support import _PublicI2CDeviceBase, _PublicUARTDeviceBase, _I2CDeviceBase, _UARTDeviceBase
from ezo_driver.errors import raise_for_result

from ._fakes import ffi as fake_ffi, lib as fake_lib


class _FakeI2CDevice(_I2CDeviceBase):
    def __init__(self, address: int = 99):
        self._transport = fake_ffi.new("ezo_fake_i2c_transport_t *")
        fake_lib.ezo_fake_i2c_transport_init(self._transport)
        self._core = ffi.new("ezo_i2c_device_t *")
        raise_for_result(
            int(
                lib.ezo_device_init(
                    self._core,
                    int(address),
                    fake_lib.ezo_fake_i2c_transport_vtable(),
                    self._transport,
                )
            )
        )

    def close(self) -> None:
        return None

    def set_response(self, payload: bytes) -> None:
        data = bytes(payload)
        fake_lib.ezo_fake_i2c_transport_set_response(self._transport, data, len(data))

    @property
    def callback_result(self) -> int:
        return int(self._transport.callback_result)

    @callback_result.setter
    def callback_result(self, result: int) -> None:
        self._transport.callback_result = int(result)

    @property
    def expected_address(self) -> int:
        return int(self._transport.expected_address)

    @expected_address.setter
    def expected_address(self, address: int) -> None:
        self._transport.expected_address = int(address)

    @property
    def enforce_expected_address(self) -> bool:
        return bool(self._transport.enforce_expected_address)

    @enforce_expected_address.setter
    def enforce_expected_address(self, enabled: bool) -> None:
        self._transport.enforce_expected_address = int(bool(enabled))

    @property
    def call_count(self) -> int:
        return int(self._transport.call_count)

    @property
    def last_tx(self) -> bytes:
        return bytes(fake_ffi.buffer(self._transport.last_tx_bytes, int(self._transport.last_tx_len)))

    @property
    def last_rx_len_requested(self) -> int:
        return int(self._transport.last_rx_len_requested)


class FakeI2CDevice(_PublicI2CDeviceBase):
    def __init__(self, address: int = 99):
        self._cdev = _FakeI2CDevice(address)

    def set_response(self, payload: bytes) -> None:
        self._cdev.set_response(payload)

    @property
    def callback_result(self) -> int:
        return self._cdev.callback_result

    @callback_result.setter
    def callback_result(self, result: int) -> None:
        self._cdev.callback_result = result

    @property
    def expected_address(self) -> int:
        return self._cdev.expected_address

    @expected_address.setter
    def expected_address(self, address: int) -> None:
        self._cdev.expected_address = address

    @property
    def enforce_expected_address(self) -> bool:
        return self._cdev.enforce_expected_address

    @enforce_expected_address.setter
    def enforce_expected_address(self, enabled: bool) -> None:
        self._cdev.enforce_expected_address = enabled

    @property
    def call_count(self) -> int:
        return self._cdev.call_count

    @property
    def last_tx(self) -> bytes:
        return self._cdev.last_tx

    @property
    def last_rx_len_requested(self) -> int:
        return self._cdev.last_rx_len_requested


class _FakeUARTDevice(_UARTDeviceBase):
    def __init__(self):
        self._transport = fake_ffi.new("ezo_fake_uart_transport_t *")
        fake_lib.ezo_fake_uart_transport_init(self._transport)
        self._core = ffi.new("ezo_uart_device_t *")
        raise_for_result(
            int(lib.ezo_uart_device_init(self._core, fake_lib.ezo_fake_uart_transport_vtable(), self._transport))
        )

    def close(self) -> None:
        return None

    def set_response(self, payload: bytes) -> None:
        data = bytes(payload)
        fake_lib.ezo_fake_uart_transport_set_response(self._transport, data, len(data))

    def append_response(self, payload: bytes) -> None:
        data = bytes(payload)
        fake_lib.ezo_fake_uart_transport_append_response(self._transport, data, len(data))

    @property
    def write_result(self) -> int:
        return int(self._transport.write_result)

    @write_result.setter
    def write_result(self, result: int) -> None:
        self._transport.write_result = int(result)

    @property
    def read_result(self) -> int:
        return int(self._transport.read_result)

    @read_result.setter
    def read_result(self, result: int) -> None:
        self._transport.read_result = int(result)

    @property
    def discard_result(self) -> int:
        return int(self._transport.discard_result)

    @discard_result.setter
    def discard_result(self, result: int) -> None:
        self._transport.discard_result = int(result)

    @property
    def max_bytes_per_read(self) -> int:
        return int(self._transport.max_bytes_per_read)

    @max_bytes_per_read.setter
    def max_bytes_per_read(self, value: int) -> None:
        self._transport.max_bytes_per_read = int(value)

    @property
    def write_call_count(self) -> int:
        return int(self._transport.write_call_count)

    @property
    def read_call_count(self) -> int:
        return int(self._transport.read_call_count)

    @property
    def discard_call_count(self) -> int:
        return int(self._transport.discard_call_count)

    @property
    def tx_bytes(self) -> bytes:
        return bytes(fake_ffi.buffer(self._transport.tx_bytes, int(self._transport.tx_len)))


class FakeUARTDevice(_PublicUARTDeviceBase):
    def __init__(self):
        self._cdev = _FakeUARTDevice()

    def set_response(self, payload: bytes) -> None:
        self._cdev.set_response(payload)

    def append_response(self, payload: bytes) -> None:
        self._cdev.append_response(payload)

    @property
    def write_result(self) -> int:
        return self._cdev.write_result

    @write_result.setter
    def write_result(self, result: int) -> None:
        self._cdev.write_result = result

    @property
    def read_result(self) -> int:
        return self._cdev.read_result

    @read_result.setter
    def read_result(self, result: int) -> None:
        self._cdev.read_result = result

    @property
    def discard_result(self) -> int:
        return self._cdev.discard_result

    @discard_result.setter
    def discard_result(self, result: int) -> None:
        self._cdev.discard_result = result

    @property
    def max_bytes_per_read(self) -> int:
        return self._cdev.max_bytes_per_read

    @max_bytes_per_read.setter
    def max_bytes_per_read(self, value: int) -> None:
        self._cdev.max_bytes_per_read = value

    @property
    def write_call_count(self) -> int:
        return self._cdev.write_call_count

    @property
    def read_call_count(self) -> int:
        return self._cdev.read_call_count

    @property
    def discard_call_count(self) -> int:
        return self._cdev.discard_call_count

    @property
    def tx_bytes(self) -> bytes:
        return self._cdev.tx_bytes


def i2c_response(status: int, payload: bytes | str = b"", *, terminate: bool = True) -> bytes:
    data = payload.encode("ascii") if isinstance(payload, str) else bytes(payload)
    if terminate:
        data += b"\0"
    return bytes([int(status)]) + data


def uart_lines(*lines: bytes | str) -> bytes:
    parts: list[bytes] = []
    for line in lines:
        parts.append(line.encode("ascii") if isinstance(line, str) else bytes(line))
    return b"\r".join(parts) + b"\r"
