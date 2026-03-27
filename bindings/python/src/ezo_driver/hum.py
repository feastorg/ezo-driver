from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    ffi,
    hum_output_config_from_c,
    hum_reading_from_c,
    hum_temperature_calibration_status_from_c,
    lib,
    read_struct,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device
from .errors import raise_for_result


def parse_reading(text: str, enabled_mask: int):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_hum_reading_t *")
    raise_for_result(int(lib.ezo_hum_parse_reading(payload, len(payload), int(enabled_mask), out)))
    return hum_reading_from_c(out[0])


def parse_output_config(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_hum_output_config_t *")
    raise_for_result(int(lib.ezo_hum_parse_output_config(payload, len(payload), out)))
    return hum_output_config_from_c(out[0])


def parse_temperature_calibration_status(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_hum_temperature_calibration_status_t *")
    raise_for_result(int(lib.ezo_hum_parse_temperature_calibration_status(payload, len(payload), out)))
    return hum_temperature_calibration_status_from_c(out[0])


def build_output_command(output: int, enabled: bool) -> str:
    return build_text(lib.ezo_hum_build_output_command, 32, int(output), int(bool(enabled)))


def build_temperature_calibration_command(temperature_c: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_hum_build_temperature_calibration_command, 32, float(temperature_c), int(decimals))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(device, *args, timing)))
    return int(timing.wait_ms)


def send_read_i2c(device) -> int:
    return _timed_send(lib.ezo_hum_send_read_i2c, _unwrap_i2c_device(device))


def send_output_query_i2c(device) -> int:
    return _timed_send(lib.ezo_hum_send_output_query_i2c, _unwrap_i2c_device(device))


def send_output_set_i2c(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_hum_send_output_set_i2c, _unwrap_i2c_device(device), int(output), int(bool(enabled)))


def send_temperature_calibration_query_i2c(device) -> int:
    return _timed_send(lib.ezo_hum_send_temperature_calibration_query_i2c, _unwrap_i2c_device(device))


def send_temperature_calibration_i2c(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_hum_send_temperature_calibration_i2c, _unwrap_i2c_device(device), float(temperature_c), int(decimals))


def send_clear_temperature_calibration_i2c(device) -> int:
    return _timed_send(lib.ezo_hum_send_clear_temperature_calibration_i2c, _unwrap_i2c_device(device))


def read_response_i2c(device, enabled_mask: int):
    return read_struct(lib.ezo_hum_read_response_i2c, "ezo_hum_reading_t", hum_reading_from_c, _unwrap_i2c_device(device), int(enabled_mask))


def read_output_config_i2c(device):
    return read_struct(lib.ezo_hum_read_output_config_i2c, "ezo_hum_output_config_t", hum_output_config_from_c, _unwrap_i2c_device(device))


def read_temperature_calibration_status_i2c(device):
    return read_struct(lib.ezo_hum_read_temperature_calibration_status_i2c, "ezo_hum_temperature_calibration_status_t", hum_temperature_calibration_status_from_c, _unwrap_i2c_device(device))


def send_read_uart(device) -> int:
    return _timed_send(lib.ezo_hum_send_read_uart, _unwrap_uart_device(device))


def send_output_query_uart(device) -> int:
    return _timed_send(lib.ezo_hum_send_output_query_uart, _unwrap_uart_device(device))


def send_output_set_uart(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_hum_send_output_set_uart, _unwrap_uart_device(device), int(output), int(bool(enabled)))


def send_temperature_calibration_query_uart(device) -> int:
    return _timed_send(lib.ezo_hum_send_temperature_calibration_query_uart, _unwrap_uart_device(device))


def send_temperature_calibration_uart(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_hum_send_temperature_calibration_uart, _unwrap_uart_device(device), float(temperature_c), int(decimals))


def send_clear_temperature_calibration_uart(device) -> int:
    return _timed_send(lib.ezo_hum_send_clear_temperature_calibration_uart, _unwrap_uart_device(device))


def read_response_uart(device, enabled_mask: int):
    return read_struct(lib.ezo_hum_read_response_uart, "ezo_hum_reading_t", hum_reading_from_c, _unwrap_uart_device(device), int(enabled_mask))


def read_output_config_uart(device):
    return read_struct(lib.ezo_hum_read_output_config_uart, "ezo_hum_output_config_t", hum_output_config_from_c, _unwrap_uart_device(device))


def read_temperature_calibration_status_uart(device):
    return read_struct(lib.ezo_hum_read_temperature_calibration_status_uart, "ezo_hum_temperature_calibration_status_t", hum_temperature_calibration_status_from_c, _unwrap_uart_device(device))
