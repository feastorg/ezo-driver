from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    do_calibration_status_from_c,
    do_output_config_from_c,
    do_pressure_compensation_from_c,
    do_reading_from_c,
    do_salinity_compensation_from_c,
    do_temperature_compensation_from_c,
    ffi,
    lib,
    read_struct,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device
from .errors import raise_for_result


def parse_reading(text: str, enabled_mask: int):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_do_reading_t *")
    raise_for_result(int(lib.ezo_do_parse_reading(payload, len(payload), int(enabled_mask), out)))
    return do_reading_from_c(out[0])


def parse_output_config(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_do_output_config_t *")
    raise_for_result(int(lib.ezo_do_parse_output_config(payload, len(payload), out)))
    return do_output_config_from_c(out[0])


def parse_temperature(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_do_temperature_compensation_t *")
    raise_for_result(int(lib.ezo_do_parse_temperature(payload, len(payload), out)))
    return do_temperature_compensation_from_c(out[0])


def parse_salinity(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_do_salinity_compensation_t *")
    raise_for_result(int(lib.ezo_do_parse_salinity(payload, len(payload), out)))
    return do_salinity_compensation_from_c(out[0])


def parse_pressure(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_do_pressure_compensation_t *")
    raise_for_result(int(lib.ezo_do_parse_pressure(payload, len(payload), out)))
    return do_pressure_compensation_from_c(out[0])


def parse_calibration_status(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_do_calibration_status_t *")
    raise_for_result(int(lib.ezo_do_parse_calibration_status(payload, len(payload), out)))
    return do_calibration_status_from_c(out[0])


def build_output_command(output: int, enabled: bool) -> str:
    return build_text(lib.ezo_do_build_output_command, 32, int(output), int(bool(enabled)))


def build_temperature_command(temperature_c: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_do_build_temperature_command, 32, float(temperature_c), int(decimals))


def build_salinity_command(value: float, unit, decimals: int = 2) -> str:
    return build_text(lib.ezo_do_build_salinity_command, 32, float(value), int(unit), int(decimals))


def build_pressure_command(pressure_kpa: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_do_build_pressure_command, 32, float(pressure_kpa), int(decimals))


def build_calibration_command(point) -> str:
    return build_text(lib.ezo_do_build_calibration_command, 32, int(point))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(device, *args, timing)))
    return int(timing.wait_ms)


def send_read_i2c(device) -> int:
    return _timed_send(lib.ezo_do_send_read_i2c, _unwrap_i2c_device(device))


def send_read_with_temp_comp_i2c(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_do_send_read_with_temp_comp_i2c, _unwrap_i2c_device(device), float(temperature_c), int(decimals))


def send_output_query_i2c(device) -> int:
    return _timed_send(lib.ezo_do_send_output_query_i2c, _unwrap_i2c_device(device))


def send_output_set_i2c(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_do_send_output_set_i2c, _unwrap_i2c_device(device), int(output), int(bool(enabled)))


def send_temperature_query_i2c(device) -> int:
    return _timed_send(lib.ezo_do_send_temperature_query_i2c, _unwrap_i2c_device(device))


def send_temperature_set_i2c(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_do_send_temperature_set_i2c, _unwrap_i2c_device(device), float(temperature_c), int(decimals))


def send_salinity_query_i2c(device) -> int:
    return _timed_send(lib.ezo_do_send_salinity_query_i2c, _unwrap_i2c_device(device))


def send_salinity_set_i2c(device, value: float, unit, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_do_send_salinity_set_i2c, _unwrap_i2c_device(device), float(value), int(unit), int(decimals))


def send_pressure_query_i2c(device) -> int:
    return _timed_send(lib.ezo_do_send_pressure_query_i2c, _unwrap_i2c_device(device))


def send_pressure_set_i2c(device, pressure_kpa: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_do_send_pressure_set_i2c, _unwrap_i2c_device(device), float(pressure_kpa), int(decimals))


def send_calibration_query_i2c(device) -> int:
    return _timed_send(lib.ezo_do_send_calibration_query_i2c, _unwrap_i2c_device(device))


def send_calibration_i2c(device, point) -> int:
    return _timed_send(lib.ezo_do_send_calibration_i2c, _unwrap_i2c_device(device), int(point))


def send_clear_calibration_i2c(device) -> int:
    return _timed_send(lib.ezo_do_send_clear_calibration_i2c, _unwrap_i2c_device(device))


def read_response_i2c(device, enabled_mask: int):
    return read_struct(lib.ezo_do_read_response_i2c, "ezo_do_reading_t", do_reading_from_c, _unwrap_i2c_device(device), int(enabled_mask))


def read_output_config_i2c(device):
    return read_struct(lib.ezo_do_read_output_config_i2c, "ezo_do_output_config_t", do_output_config_from_c, _unwrap_i2c_device(device))


def read_temperature_i2c(device):
    return read_struct(lib.ezo_do_read_temperature_i2c, "ezo_do_temperature_compensation_t", do_temperature_compensation_from_c, _unwrap_i2c_device(device))


def read_salinity_i2c(device):
    return read_struct(lib.ezo_do_read_salinity_i2c, "ezo_do_salinity_compensation_t", do_salinity_compensation_from_c, _unwrap_i2c_device(device))


def read_pressure_i2c(device):
    return read_struct(lib.ezo_do_read_pressure_i2c, "ezo_do_pressure_compensation_t", do_pressure_compensation_from_c, _unwrap_i2c_device(device))


def read_calibration_status_i2c(device):
    return read_struct(lib.ezo_do_read_calibration_status_i2c, "ezo_do_calibration_status_t", do_calibration_status_from_c, _unwrap_i2c_device(device))


def send_read_uart(device) -> int:
    return _timed_send(lib.ezo_do_send_read_uart, _unwrap_uart_device(device))


def send_read_with_temp_comp_uart(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_do_send_read_with_temp_comp_uart, _unwrap_uart_device(device), float(temperature_c), int(decimals))


def send_output_query_uart(device) -> int:
    return _timed_send(lib.ezo_do_send_output_query_uart, _unwrap_uart_device(device))


def send_output_set_uart(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_do_send_output_set_uart, _unwrap_uart_device(device), int(output), int(bool(enabled)))


def send_temperature_query_uart(device) -> int:
    return _timed_send(lib.ezo_do_send_temperature_query_uart, _unwrap_uart_device(device))


def send_temperature_set_uart(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_do_send_temperature_set_uart, _unwrap_uart_device(device), float(temperature_c), int(decimals))


def send_salinity_query_uart(device) -> int:
    return _timed_send(lib.ezo_do_send_salinity_query_uart, _unwrap_uart_device(device))


def send_salinity_set_uart(device, value: float, unit, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_do_send_salinity_set_uart, _unwrap_uart_device(device), float(value), int(unit), int(decimals))


def send_pressure_query_uart(device) -> int:
    return _timed_send(lib.ezo_do_send_pressure_query_uart, _unwrap_uart_device(device))


def send_pressure_set_uart(device, pressure_kpa: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_do_send_pressure_set_uart, _unwrap_uart_device(device), float(pressure_kpa), int(decimals))


def send_calibration_query_uart(device) -> int:
    return _timed_send(lib.ezo_do_send_calibration_query_uart, _unwrap_uart_device(device))


def send_calibration_uart(device, point) -> int:
    return _timed_send(lib.ezo_do_send_calibration_uart, _unwrap_uart_device(device), int(point))


def send_clear_calibration_uart(device) -> int:
    return _timed_send(lib.ezo_do_send_clear_calibration_uart, _unwrap_uart_device(device))


def read_response_uart(device, enabled_mask: int):
    return read_struct(lib.ezo_do_read_response_uart, "ezo_do_reading_t", do_reading_from_c, _unwrap_uart_device(device), int(enabled_mask))


def read_output_config_uart(device):
    return read_struct(lib.ezo_do_read_output_config_uart, "ezo_do_output_config_t", do_output_config_from_c, _unwrap_uart_device(device))


def read_temperature_uart(device):
    return read_struct(lib.ezo_do_read_temperature_uart, "ezo_do_temperature_compensation_t", do_temperature_compensation_from_c, _unwrap_uart_device(device))


def read_salinity_uart(device):
    return read_struct(lib.ezo_do_read_salinity_uart, "ezo_do_salinity_compensation_t", do_salinity_compensation_from_c, _unwrap_uart_device(device))


def read_pressure_uart(device):
    return read_struct(lib.ezo_do_read_pressure_uart, "ezo_do_pressure_compensation_t", do_pressure_compensation_from_c, _unwrap_uart_device(device))


def read_calibration_status_uart(device):
    return read_struct(lib.ezo_do_read_calibration_status_uart, "ezo_do_calibration_status_t", do_calibration_status_from_c, _unwrap_uart_device(device))
