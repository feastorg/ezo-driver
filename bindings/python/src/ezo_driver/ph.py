from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    ffi,
    lib,
    ph_calibration_status_from_c,
    ph_extended_range_status_from_c,
    ph_reading_from_c,
    ph_slope_from_c,
    ph_temperature_compensation_from_c,
    read_struct,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device
from .errors import raise_for_result


def parse_reading(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ph_reading_t *")
    raise_for_result(int(lib.ezo_ph_parse_reading(payload, len(payload), out)))
    return ph_reading_from_c(out[0])


def parse_temperature(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ph_temperature_compensation_t *")
    raise_for_result(int(lib.ezo_ph_parse_temperature(payload, len(payload), out)))
    return ph_temperature_compensation_from_c(out[0])


def parse_calibration_status(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ph_calibration_status_t *")
    raise_for_result(int(lib.ezo_ph_parse_calibration_status(payload, len(payload), out)))
    return ph_calibration_status_from_c(out[0])


def parse_slope(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ph_slope_t *")
    raise_for_result(int(lib.ezo_ph_parse_slope(payload, len(payload), out)))
    return ph_slope_from_c(out[0])


def parse_extended_range(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ph_extended_range_status_t *")
    raise_for_result(int(lib.ezo_ph_parse_extended_range(payload, len(payload), out)))
    return ph_extended_range_status_from_c(out[0])


def build_temperature_command(temperature_c: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_ph_build_temperature_command, 32, float(temperature_c), int(decimals))


def build_calibration_command(point, reference_ph: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_ph_build_calibration_command, 32, int(point), float(reference_ph), int(decimals))


def build_extended_range_command(enabled) -> str:
    return build_text(lib.ezo_ph_build_extended_range_command, 32, int(enabled))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(device, *args, timing)))
    return int(timing.wait_ms)


def send_read_i2c(device) -> int:
    return _timed_send(lib.ezo_ph_send_read_i2c, _unwrap_i2c_device(device))


def send_read_with_temp_comp_i2c(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ph_send_read_with_temp_comp_i2c, _unwrap_i2c_device(device), float(temperature_c), int(decimals))


def send_temperature_query_i2c(device) -> int:
    return _timed_send(lib.ezo_ph_send_temperature_query_i2c, _unwrap_i2c_device(device))


def send_temperature_set_i2c(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ph_send_temperature_set_i2c, _unwrap_i2c_device(device), float(temperature_c), int(decimals))


def send_calibration_query_i2c(device) -> int:
    return _timed_send(lib.ezo_ph_send_calibration_query_i2c, _unwrap_i2c_device(device))


def send_calibration_i2c(device, point, reference_ph: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ph_send_calibration_i2c, _unwrap_i2c_device(device), int(point), float(reference_ph), int(decimals))


def send_clear_calibration_i2c(device) -> int:
    return _timed_send(lib.ezo_ph_send_clear_calibration_i2c, _unwrap_i2c_device(device))


def send_slope_query_i2c(device) -> int:
    return _timed_send(lib.ezo_ph_send_slope_query_i2c, _unwrap_i2c_device(device))


def send_extended_range_query_i2c(device) -> int:
    return _timed_send(lib.ezo_ph_send_extended_range_query_i2c, _unwrap_i2c_device(device))


def send_extended_range_set_i2c(device, enabled) -> int:
    return _timed_send(lib.ezo_ph_send_extended_range_set_i2c, _unwrap_i2c_device(device), int(enabled))


def read_response_i2c(device):
    return read_struct(lib.ezo_ph_read_response_i2c, "ezo_ph_reading_t", ph_reading_from_c, _unwrap_i2c_device(device))


def read_temperature_i2c(device):
    return read_struct(lib.ezo_ph_read_temperature_i2c, "ezo_ph_temperature_compensation_t", ph_temperature_compensation_from_c, _unwrap_i2c_device(device))


def read_calibration_status_i2c(device):
    return read_struct(lib.ezo_ph_read_calibration_status_i2c, "ezo_ph_calibration_status_t", ph_calibration_status_from_c, _unwrap_i2c_device(device))


def read_slope_i2c(device):
    return read_struct(lib.ezo_ph_read_slope_i2c, "ezo_ph_slope_t", ph_slope_from_c, _unwrap_i2c_device(device))


def read_extended_range_i2c(device):
    return read_struct(lib.ezo_ph_read_extended_range_i2c, "ezo_ph_extended_range_status_t", ph_extended_range_status_from_c, _unwrap_i2c_device(device))


def send_read_uart(device) -> int:
    return _timed_send(lib.ezo_ph_send_read_uart, _unwrap_uart_device(device))


def send_read_with_temp_comp_uart(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ph_send_read_with_temp_comp_uart, _unwrap_uart_device(device), float(temperature_c), int(decimals))


def send_temperature_query_uart(device) -> int:
    return _timed_send(lib.ezo_ph_send_temperature_query_uart, _unwrap_uart_device(device))


def send_temperature_set_uart(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ph_send_temperature_set_uart, _unwrap_uart_device(device), float(temperature_c), int(decimals))


def send_calibration_query_uart(device) -> int:
    return _timed_send(lib.ezo_ph_send_calibration_query_uart, _unwrap_uart_device(device))


def send_calibration_uart(device, point, reference_ph: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ph_send_calibration_uart, _unwrap_uart_device(device), int(point), float(reference_ph), int(decimals))


def send_clear_calibration_uart(device) -> int:
    return _timed_send(lib.ezo_ph_send_clear_calibration_uart, _unwrap_uart_device(device))


def send_slope_query_uart(device) -> int:
    return _timed_send(lib.ezo_ph_send_slope_query_uart, _unwrap_uart_device(device))


def send_extended_range_query_uart(device) -> int:
    return _timed_send(lib.ezo_ph_send_extended_range_query_uart, _unwrap_uart_device(device))


def send_extended_range_set_uart(device, enabled) -> int:
    return _timed_send(lib.ezo_ph_send_extended_range_set_uart, _unwrap_uart_device(device), int(enabled))


def read_response_uart(device):
    return read_struct(lib.ezo_ph_read_response_uart, "ezo_ph_reading_t", ph_reading_from_c, _unwrap_uart_device(device))


def read_response_with_temp_comp_uart(device):
    return read_struct(lib.ezo_ph_read_response_with_temp_comp_uart, "ezo_ph_reading_t", ph_reading_from_c, _unwrap_uart_device(device))


def read_temperature_uart(device):
    return read_struct(lib.ezo_ph_read_temperature_uart, "ezo_ph_temperature_compensation_t", ph_temperature_compensation_from_c, _unwrap_uart_device(device))


def read_calibration_status_uart(device):
    return read_struct(lib.ezo_ph_read_calibration_status_uart, "ezo_ph_calibration_status_t", ph_calibration_status_from_c, _unwrap_uart_device(device))


def read_slope_uart(device):
    return read_struct(lib.ezo_ph_read_slope_uart, "ezo_ph_slope_t", ph_slope_from_c, _unwrap_uart_device(device))


def read_extended_range_uart(device):
    return read_struct(lib.ezo_ph_read_extended_range_uart, "ezo_ph_extended_range_status_t", ph_extended_range_status_from_c, _unwrap_uart_device(device))
