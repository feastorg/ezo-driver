from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    ffi,
    lib,
    o2_output_config_from_c,
    o2_reading_from_c,
    o2_temperature_compensation_from_c,
    read_struct,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device
from .errors import raise_for_result
from .types import O2OutputConfig, O2Reading, O2TemperatureCompensation


def parse_reading(text: str, enabled_mask: int) -> O2Reading:
    payload = ascii_bytes(text)
    out = ffi.new("ezo_o2_reading_t *")
    raise_for_result(int(lib.ezo_o2_parse_reading(payload, len(payload), int(enabled_mask), out)))
    return o2_reading_from_c(out[0])


def parse_output_config(text: str) -> O2OutputConfig:
    payload = ascii_bytes(text)
    out = ffi.new("ezo_o2_output_config_t *")
    raise_for_result(int(lib.ezo_o2_parse_output_config(payload, len(payload), out)))
    return o2_output_config_from_c(out[0])


def parse_temperature(text: str) -> O2TemperatureCompensation:
    payload = ascii_bytes(text)
    out = ffi.new("ezo_o2_temperature_compensation_t *")
    raise_for_result(int(lib.ezo_o2_parse_temperature(payload, len(payload), out)))
    return o2_temperature_compensation_from_c(out[0])


def build_output_command(output: int, enabled: bool) -> str:
    return build_text(lib.ezo_o2_build_output_command, 32, int(output), int(bool(enabled)))


def build_temperature_command(temperature_c: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_o2_build_temperature_command, 32, float(temperature_c), int(decimals))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(device, *args, timing)), device=device)
    return int(timing.wait_ms)


def send_read_i2c(device) -> int:
    return _timed_send(lib.ezo_o2_send_read_i2c, _unwrap_i2c_device(device))


def send_read_with_temp_comp_i2c(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_o2_send_read_with_temp_comp_i2c, _unwrap_i2c_device(device), float(temperature_c), int(decimals))


def send_output_query_i2c(device) -> int:
    return _timed_send(lib.ezo_o2_send_output_query_i2c, _unwrap_i2c_device(device))


def send_output_set_i2c(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_o2_send_output_set_i2c, _unwrap_i2c_device(device), int(output), int(bool(enabled)))


def send_temperature_query_i2c(device) -> int:
    return _timed_send(lib.ezo_o2_send_temperature_query_i2c, _unwrap_i2c_device(device))


def send_temperature_set_i2c(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_o2_send_temperature_set_i2c, _unwrap_i2c_device(device), float(temperature_c), int(decimals))


def read_response_i2c(device, enabled_mask: int) -> O2Reading:
    return read_struct(lib.ezo_o2_read_response_i2c, "ezo_o2_reading_t", o2_reading_from_c, _unwrap_i2c_device(device), int(enabled_mask))


def read_output_config_i2c(device) -> O2OutputConfig:
    return read_struct(lib.ezo_o2_read_output_config_i2c, "ezo_o2_output_config_t", o2_output_config_from_c, _unwrap_i2c_device(device))


def read_temperature_i2c(device) -> O2TemperatureCompensation:
    return read_struct(lib.ezo_o2_read_temperature_i2c, "ezo_o2_temperature_compensation_t", o2_temperature_compensation_from_c, _unwrap_i2c_device(device))


def send_read_uart(device) -> int:
    return _timed_send(lib.ezo_o2_send_read_uart, _unwrap_uart_device(device))


def send_read_with_temp_comp_uart(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_o2_send_read_with_temp_comp_uart, _unwrap_uart_device(device), float(temperature_c), int(decimals))


def send_output_query_uart(device) -> int:
    return _timed_send(lib.ezo_o2_send_output_query_uart, _unwrap_uart_device(device))


def send_output_set_uart(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_o2_send_output_set_uart, _unwrap_uart_device(device), int(output), int(bool(enabled)))


def send_temperature_query_uart(device) -> int:
    return _timed_send(lib.ezo_o2_send_temperature_query_uart, _unwrap_uart_device(device))


def send_temperature_set_uart(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_o2_send_temperature_set_uart, _unwrap_uart_device(device), float(temperature_c), int(decimals))


def read_response_uart(device, enabled_mask: int) -> O2Reading:
    return read_struct(lib.ezo_o2_read_response_uart, "ezo_o2_reading_t", o2_reading_from_c, _unwrap_uart_device(device), int(enabled_mask))


def read_response_with_temp_comp_uart(device, enabled_mask: int) -> O2Reading:
    return read_struct(lib.ezo_o2_read_response_with_temp_comp_uart, "ezo_o2_reading_t", o2_reading_from_c, _unwrap_uart_device(device), int(enabled_mask))


def read_output_config_uart(device) -> O2OutputConfig:
    return read_struct(lib.ezo_o2_read_output_config_uart, "ezo_o2_output_config_t", o2_output_config_from_c, _unwrap_uart_device(device))


def read_temperature_uart(device) -> O2TemperatureCompensation:
    return read_struct(lib.ezo_o2_read_temperature_uart, "ezo_o2_temperature_compensation_t", o2_temperature_compensation_from_c, _unwrap_uart_device(device))
