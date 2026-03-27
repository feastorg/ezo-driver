from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    ffi,
    lib,
    orp_calibration_status_from_c,
    orp_extended_scale_status_from_c,
    orp_reading_from_c,
    read_struct,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device
from .errors import raise_for_result


def parse_reading(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_orp_reading_t *")
    raise_for_result(int(lib.ezo_orp_parse_reading(payload, len(payload), out)))
    return orp_reading_from_c(out[0])


def parse_calibration_status(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_orp_calibration_status_t *")
    raise_for_result(int(lib.ezo_orp_parse_calibration_status(payload, len(payload), out)))
    return orp_calibration_status_from_c(out[0])


def parse_extended_scale(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_orp_extended_scale_status_t *")
    raise_for_result(int(lib.ezo_orp_parse_extended_scale(payload, len(payload), out)))
    return orp_extended_scale_status_from_c(out[0])


def build_calibration_command(reference_mv: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_orp_build_calibration_command, 32, float(reference_mv), int(decimals))


def build_extended_scale_command(enabled) -> str:
    return build_text(lib.ezo_orp_build_extended_scale_command, 32, int(enabled))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(device, *args, timing)))
    return int(timing.wait_ms)


def send_read_i2c(device) -> int:
    return _timed_send(lib.ezo_orp_send_read_i2c, _unwrap_i2c_device(device))


def send_calibration_query_i2c(device) -> int:
    return _timed_send(lib.ezo_orp_send_calibration_query_i2c, _unwrap_i2c_device(device))


def send_calibration_i2c(device, reference_mv: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_orp_send_calibration_i2c, _unwrap_i2c_device(device), float(reference_mv), int(decimals))


def send_clear_calibration_i2c(device) -> int:
    return _timed_send(lib.ezo_orp_send_clear_calibration_i2c, _unwrap_i2c_device(device))


def send_extended_scale_query_i2c(device) -> int:
    return _timed_send(lib.ezo_orp_send_extended_scale_query_i2c, _unwrap_i2c_device(device))


def send_extended_scale_set_i2c(device, enabled) -> int:
    return _timed_send(lib.ezo_orp_send_extended_scale_set_i2c, _unwrap_i2c_device(device), int(enabled))


def read_response_i2c(device):
    return read_struct(lib.ezo_orp_read_response_i2c, "ezo_orp_reading_t", orp_reading_from_c, _unwrap_i2c_device(device))


def read_calibration_status_i2c(device):
    return read_struct(lib.ezo_orp_read_calibration_status_i2c, "ezo_orp_calibration_status_t", orp_calibration_status_from_c, _unwrap_i2c_device(device))


def read_extended_scale_i2c(device):
    return read_struct(lib.ezo_orp_read_extended_scale_i2c, "ezo_orp_extended_scale_status_t", orp_extended_scale_status_from_c, _unwrap_i2c_device(device))


def send_read_uart(device) -> int:
    return _timed_send(lib.ezo_orp_send_read_uart, _unwrap_uart_device(device))


def send_calibration_query_uart(device) -> int:
    return _timed_send(lib.ezo_orp_send_calibration_query_uart, _unwrap_uart_device(device))


def send_calibration_uart(device, reference_mv: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_orp_send_calibration_uart, _unwrap_uart_device(device), float(reference_mv), int(decimals))


def send_clear_calibration_uart(device) -> int:
    return _timed_send(lib.ezo_orp_send_clear_calibration_uart, _unwrap_uart_device(device))


def send_extended_scale_query_uart(device) -> int:
    return _timed_send(lib.ezo_orp_send_extended_scale_query_uart, _unwrap_uart_device(device))


def send_extended_scale_set_uart(device, enabled) -> int:
    return _timed_send(lib.ezo_orp_send_extended_scale_set_uart, _unwrap_uart_device(device), int(enabled))


def read_response_uart(device):
    return read_struct(lib.ezo_orp_read_response_uart, "ezo_orp_reading_t", orp_reading_from_c, _unwrap_uart_device(device))


def read_calibration_status_uart(device):
    return read_struct(lib.ezo_orp_read_calibration_status_uart, "ezo_orp_calibration_status_t", orp_calibration_status_from_c, _unwrap_uart_device(device))


def read_extended_scale_uart(device):
    return read_struct(lib.ezo_orp_read_extended_scale_uart, "ezo_orp_extended_scale_status_t", orp_extended_scale_status_from_c, _unwrap_uart_device(device))
