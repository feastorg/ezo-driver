from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    ffi,
    lib,
    read_struct,
    rtd_calibration_status_from_c,
    rtd_logger_status_from_c,
    rtd_memory_entry_from_c,
    rtd_memory_status_from_c,
    rtd_memory_values_from_c,
    rtd_reading_from_c,
    rtd_scale_status_from_c,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device
from .errors import raise_for_result


def parse_reading(text: str, scale):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_rtd_reading_t *")
    raise_for_result(int(lib.ezo_rtd_parse_reading(payload, len(payload), int(scale), out)))
    return rtd_reading_from_c(out[0])


def parse_scale(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_rtd_scale_status_t *")
    raise_for_result(int(lib.ezo_rtd_parse_scale(payload, len(payload), out)))
    return rtd_scale_status_from_c(out[0])


def parse_calibration_status(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_rtd_calibration_status_t *")
    raise_for_result(int(lib.ezo_rtd_parse_calibration_status(payload, len(payload), out)))
    return rtd_calibration_status_from_c(out[0])


def parse_logger_status(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_rtd_logger_status_t *")
    raise_for_result(int(lib.ezo_rtd_parse_logger_status(payload, len(payload), out)))
    return rtd_logger_status_from_c(out[0])


def parse_memory_status(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_rtd_memory_status_t *")
    raise_for_result(int(lib.ezo_rtd_parse_memory_status(payload, len(payload), out)))
    return rtd_memory_status_from_c(out[0])


def parse_memory_entry(text: str, scale):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_rtd_memory_entry_t *")
    raise_for_result(int(lib.ezo_rtd_parse_memory_entry(payload, len(payload), int(scale), out)))
    return rtd_memory_entry_from_c(out[0])


def parse_memory_all(text: str, scale) -> list:
    payload = ascii_bytes(text)
    values = ffi.new("ezo_rtd_memory_value_t[128]")
    count = ffi.new("size_t *")
    raise_for_result(int(lib.ezo_rtd_parse_memory_all(payload, len(payload), int(scale), values, 128, count)))
    return rtd_memory_values_from_c(values, count[0])


def build_scale_command(scale) -> str:
    return build_text(lib.ezo_rtd_build_scale_command, 32, int(scale))


def build_calibration_command(reference_temperature: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_rtd_build_calibration_command, 32, float(reference_temperature), int(decimals))


def build_logger_command(interval_units: int) -> str:
    return build_text(lib.ezo_rtd_build_logger_command, 32, int(interval_units))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(device, *args, timing)))
    return int(timing.wait_ms)


def send_read_i2c(device) -> int:
    return _timed_send(lib.ezo_rtd_send_read_i2c, _unwrap_i2c_device(device))


def send_scale_query_i2c(device) -> int:
    return _timed_send(lib.ezo_rtd_send_scale_query_i2c, _unwrap_i2c_device(device))


def send_scale_set_i2c(device, scale) -> int:
    return _timed_send(lib.ezo_rtd_send_scale_set_i2c, _unwrap_i2c_device(device), int(scale))


def send_calibration_query_i2c(device) -> int:
    return _timed_send(lib.ezo_rtd_send_calibration_query_i2c, _unwrap_i2c_device(device))


def send_calibration_i2c(device, reference_temperature: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_rtd_send_calibration_i2c, _unwrap_i2c_device(device), float(reference_temperature), int(decimals))


def send_clear_calibration_i2c(device) -> int:
    return _timed_send(lib.ezo_rtd_send_clear_calibration_i2c, _unwrap_i2c_device(device))


def send_logger_query_i2c(device) -> int:
    return _timed_send(lib.ezo_rtd_send_logger_query_i2c, _unwrap_i2c_device(device))


def send_logger_set_i2c(device, interval_units: int) -> int:
    return _timed_send(lib.ezo_rtd_send_logger_set_i2c, _unwrap_i2c_device(device), int(interval_units))


def send_memory_query_i2c(device) -> int:
    return _timed_send(lib.ezo_rtd_send_memory_query_i2c, _unwrap_i2c_device(device))


def send_memory_next_i2c(device) -> int:
    return _timed_send(lib.ezo_rtd_send_memory_next_i2c, _unwrap_i2c_device(device))


def send_memory_all_i2c(device) -> int:
    return _timed_send(lib.ezo_rtd_send_memory_all_i2c, _unwrap_i2c_device(device))


def send_memory_clear_i2c(device) -> int:
    return _timed_send(lib.ezo_rtd_send_memory_clear_i2c, _unwrap_i2c_device(device))


def read_response_i2c(device, scale):
    return read_struct(lib.ezo_rtd_read_response_i2c, "ezo_rtd_reading_t", rtd_reading_from_c, _unwrap_i2c_device(device), int(scale))


def read_scale_i2c(device):
    return read_struct(lib.ezo_rtd_read_scale_i2c, "ezo_rtd_scale_status_t", rtd_scale_status_from_c, _unwrap_i2c_device(device))


def read_calibration_status_i2c(device):
    return read_struct(lib.ezo_rtd_read_calibration_status_i2c, "ezo_rtd_calibration_status_t", rtd_calibration_status_from_c, _unwrap_i2c_device(device))


def read_logger_i2c(device):
    return read_struct(lib.ezo_rtd_read_logger_i2c, "ezo_rtd_logger_status_t", rtd_logger_status_from_c, _unwrap_i2c_device(device))


def read_memory_status_i2c(device):
    return read_struct(lib.ezo_rtd_read_memory_status_i2c, "ezo_rtd_memory_status_t", rtd_memory_status_from_c, _unwrap_i2c_device(device))


def read_memory_entry_i2c(device, scale):
    return read_struct(lib.ezo_rtd_read_memory_entry_i2c, "ezo_rtd_memory_entry_t", rtd_memory_entry_from_c, _unwrap_i2c_device(device), int(scale))


def read_memory_all_i2c(device, scale) -> list:
    values = ffi.new("ezo_rtd_memory_value_t[128]")
    count = ffi.new("size_t *")
    raise_for_result(int(lib.ezo_rtd_read_memory_all_i2c(_unwrap_i2c_device(device), int(scale), values, 128, count)))
    return rtd_memory_values_from_c(values, count[0])


def send_read_uart(device) -> int:
    return _timed_send(lib.ezo_rtd_send_read_uart, _unwrap_uart_device(device))


def send_scale_query_uart(device) -> int:
    return _timed_send(lib.ezo_rtd_send_scale_query_uart, _unwrap_uart_device(device))


def send_scale_set_uart(device, scale) -> int:
    return _timed_send(lib.ezo_rtd_send_scale_set_uart, _unwrap_uart_device(device), int(scale))


def send_calibration_query_uart(device) -> int:
    return _timed_send(lib.ezo_rtd_send_calibration_query_uart, _unwrap_uart_device(device))


def send_calibration_uart(device, reference_temperature: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_rtd_send_calibration_uart, _unwrap_uart_device(device), float(reference_temperature), int(decimals))


def send_clear_calibration_uart(device) -> int:
    return _timed_send(lib.ezo_rtd_send_clear_calibration_uart, _unwrap_uart_device(device))


def send_logger_query_uart(device) -> int:
    return _timed_send(lib.ezo_rtd_send_logger_query_uart, _unwrap_uart_device(device))


def send_logger_set_uart(device, interval_units: int) -> int:
    return _timed_send(lib.ezo_rtd_send_logger_set_uart, _unwrap_uart_device(device), int(interval_units))


def send_memory_query_uart(device) -> int:
    return _timed_send(lib.ezo_rtd_send_memory_query_uart, _unwrap_uart_device(device))


def send_memory_next_uart(device) -> int:
    return _timed_send(lib.ezo_rtd_send_memory_next_uart, _unwrap_uart_device(device))


def send_memory_all_uart(device) -> int:
    return _timed_send(lib.ezo_rtd_send_memory_all_uart, _unwrap_uart_device(device))


def send_memory_clear_uart(device) -> int:
    return _timed_send(lib.ezo_rtd_send_memory_clear_uart, _unwrap_uart_device(device))


def read_response_uart(device, scale):
    return read_struct(lib.ezo_rtd_read_response_uart, "ezo_rtd_reading_t", rtd_reading_from_c, _unwrap_uart_device(device), int(scale))


def read_scale_uart(device):
    return read_struct(lib.ezo_rtd_read_scale_uart, "ezo_rtd_scale_status_t", rtd_scale_status_from_c, _unwrap_uart_device(device))


def read_calibration_status_uart(device):
    return read_struct(lib.ezo_rtd_read_calibration_status_uart, "ezo_rtd_calibration_status_t", rtd_calibration_status_from_c, _unwrap_uart_device(device))


def read_logger_uart(device):
    return read_struct(lib.ezo_rtd_read_logger_uart, "ezo_rtd_logger_status_t", rtd_logger_status_from_c, _unwrap_uart_device(device))


def read_memory_status_uart(device):
    return read_struct(lib.ezo_rtd_read_memory_status_uart, "ezo_rtd_memory_status_t", rtd_memory_status_from_c, _unwrap_uart_device(device))


def read_memory_entry_uart(device, scale):
    return read_struct(lib.ezo_rtd_read_memory_entry_uart, "ezo_rtd_memory_entry_t", rtd_memory_entry_from_c, _unwrap_uart_device(device), int(scale))


def read_memory_all_uart(device, scale) -> list:
    values = ffi.new("ezo_rtd_memory_value_t[128]")
    count = ffi.new("size_t *")
    raise_for_result(int(lib.ezo_rtd_read_memory_all_uart(_unwrap_uart_device(device), int(scale), values, 128, count)))
    return rtd_memory_values_from_c(values, count[0])
