from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    ffi,
    lib,
    co2_output_config_from_c,
    co2_reading_from_c,
    read_struct,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device
from .errors import raise_for_result
from .types import CO2OutputConfig, CO2Reading


def parse_reading(text: str, enabled_mask: int) -> CO2Reading:
    payload = ascii_bytes(text)
    out = ffi.new("ezo_co2_reading_t *")
    raise_for_result(int(lib.ezo_co2_parse_reading(payload, len(payload), int(enabled_mask), out)))
    return co2_reading_from_c(out[0])


def parse_output_config(text: str) -> CO2OutputConfig:
    payload = ascii_bytes(text)
    out = ffi.new("ezo_co2_output_config_t *")
    raise_for_result(int(lib.ezo_co2_parse_output_config(payload, len(payload), out)))
    return co2_output_config_from_c(out[0])


def build_output_command(output: int, enabled: bool) -> str:
    return build_text(lib.ezo_co2_build_output_command, 32, int(output), int(bool(enabled)))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(device, *args, timing)), device=device)
    return int(timing.wait_ms)


def send_read_i2c(device) -> int:
    return _timed_send(lib.ezo_co2_send_read_i2c, _unwrap_i2c_device(device))


def send_output_query_i2c(device) -> int:
    return _timed_send(lib.ezo_co2_send_output_query_i2c, _unwrap_i2c_device(device))


def send_output_set_i2c(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_co2_send_output_set_i2c, _unwrap_i2c_device(device), int(output), int(bool(enabled)))


def read_response_i2c(device, enabled_mask: int) -> CO2Reading:
    return read_struct(lib.ezo_co2_read_response_i2c, "ezo_co2_reading_t", co2_reading_from_c, _unwrap_i2c_device(device), int(enabled_mask))


def read_output_config_i2c(device) -> CO2OutputConfig:
    return read_struct(lib.ezo_co2_read_output_config_i2c, "ezo_co2_output_config_t", co2_output_config_from_c, _unwrap_i2c_device(device))


def send_read_uart(device) -> int:
    return _timed_send(lib.ezo_co2_send_read_uart, _unwrap_uart_device(device))


def send_output_query_uart(device) -> int:
    return _timed_send(lib.ezo_co2_send_output_query_uart, _unwrap_uart_device(device))


def send_output_set_uart(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_co2_send_output_set_uart, _unwrap_uart_device(device), int(output), int(bool(enabled)))


def read_response_uart(device, enabled_mask: int) -> CO2Reading:
    return read_struct(lib.ezo_co2_read_response_uart, "ezo_co2_reading_t", co2_reading_from_c, _unwrap_uart_device(device), int(enabled_mask))


def read_output_config_uart(device) -> CO2OutputConfig:
    return read_struct(lib.ezo_co2_read_output_config_uart, "ezo_co2_output_config_t", co2_output_config_from_c, _unwrap_uart_device(device))
