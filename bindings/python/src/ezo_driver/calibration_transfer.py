from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    calibration_export_info_from_c,
    calibration_import_result_from_c,
    ffi,
    lib,
    read_struct,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device
from .enums import DeviceStatus, UARTResponseKind
from .errors import raise_for_result


def parse_export_info(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_calibration_export_info_t *")
    raise_for_result(int(lib.ezo_calibration_parse_export_info(payload, len(payload), out)))
    return calibration_export_info_from_c(out[0])


def build_import_command(payload: bytes | str) -> str:
    return build_text(lib.ezo_calibration_build_import_command, 256, ascii_bytes(payload))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(device, *args, timing)))
    return int(timing.wait_ms)


def send_export_info_query_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_calibration_send_export_info_query_i2c, _unwrap_i2c_device(device), int(product_id))


def send_export_next_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_calibration_send_export_next_i2c, _unwrap_i2c_device(device), int(product_id))


def send_import_i2c(device, product_id, payload: bytes | str) -> int:
    return _timed_send(
        lib.ezo_calibration_send_import_i2c,
        _unwrap_i2c_device(device),
        int(product_id),
        ascii_bytes(payload),
    )


def read_export_info_i2c(device):
    return read_struct(
        lib.ezo_calibration_read_export_info_i2c,
        "ezo_calibration_export_info_t",
        calibration_export_info_from_c,
        _unwrap_i2c_device(device),
    )


def read_export_chunk_i2c(device) -> bytes:
    buffer = ffi.new("char[256]")
    length = ffi.new("size_t *")
    raise_for_result(int(lib.ezo_calibration_read_export_chunk_i2c(_unwrap_i2c_device(device), buffer, 256, length)))
    return bytes(ffi.buffer(buffer, int(length[0])))


def read_import_status_i2c(device) -> DeviceStatus:
    status = ffi.new("ezo_device_status_t *")
    raise_for_result(int(lib.ezo_calibration_read_import_status_i2c(_unwrap_i2c_device(device), status)))
    return DeviceStatus(int(status[0]))


def read_import_result_i2c(device):
    return read_struct(
        lib.ezo_calibration_read_import_result_i2c,
        "ezo_calibration_import_result_t",
        calibration_import_result_from_c,
        _unwrap_i2c_device(device),
    )


def send_export_info_query_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_calibration_send_export_info_query_uart, _unwrap_uart_device(device), int(product_id))


def send_export_next_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_calibration_send_export_next_uart, _unwrap_uart_device(device), int(product_id))


def send_import_uart(device, product_id, payload: bytes | str) -> int:
    return _timed_send(
        lib.ezo_calibration_send_import_uart,
        _unwrap_uart_device(device),
        int(product_id),
        ascii_bytes(payload),
    )


def read_export_info_uart(device):
    return read_struct(
        lib.ezo_calibration_read_export_info_uart,
        "ezo_calibration_export_info_t",
        calibration_export_info_from_c,
        _unwrap_uart_device(device),
    )


def read_export_chunk_uart(device) -> tuple[UARTResponseKind, bytes]:
    buffer = ffi.new("char[256]")
    length = ffi.new("size_t *")
    kind = ffi.new("ezo_uart_response_kind_t *")
    raise_for_result(
        int(
            lib.ezo_calibration_read_export_chunk_uart(
                _unwrap_uart_device(device),
                buffer,
                256,
                length,
                kind,
            )
        )
    )
    return UARTResponseKind(int(kind[0])), bytes(ffi.buffer(buffer, int(length[0])))


def read_import_result_uart(device) -> UARTResponseKind:
    kind = ffi.new("ezo_uart_response_kind_t *")
    raise_for_result(int(lib.ezo_calibration_read_import_result_uart(_unwrap_uart_device(device), kind)))
    return UARTResponseKind(int(kind[0]))
