from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    ec_calibration_status_from_c,
    ec_output_config_from_c,
    ec_probe_k_from_c,
    ec_reading_from_c,
    ec_tds_factor_from_c,
    ec_temperature_compensation_from_c,
    ffi,
    lib,
    read_struct,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device
from .errors import raise_for_result


def parse_reading(text: str, enabled_mask: int):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ec_reading_t *")
    raise_for_result(int(lib.ezo_ec_parse_reading(payload, len(payload), int(enabled_mask), out)))
    return ec_reading_from_c(out[0])


def parse_output_config(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ec_output_config_t *")
    raise_for_result(int(lib.ezo_ec_parse_output_config(payload, len(payload), out)))
    return ec_output_config_from_c(out[0])


def parse_temperature(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ec_temperature_compensation_t *")
    raise_for_result(int(lib.ezo_ec_parse_temperature(payload, len(payload), out)))
    return ec_temperature_compensation_from_c(out[0])


def parse_probe_k(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ec_probe_k_t *")
    raise_for_result(int(lib.ezo_ec_parse_probe_k(payload, len(payload), out)))
    return ec_probe_k_from_c(out[0])


def parse_tds_factor(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ec_tds_factor_t *")
    raise_for_result(int(lib.ezo_ec_parse_tds_factor(payload, len(payload), out)))
    return ec_tds_factor_from_c(out[0])


def parse_calibration_status(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_ec_calibration_status_t *")
    raise_for_result(int(lib.ezo_ec_parse_calibration_status(payload, len(payload), out)))
    return ec_calibration_status_from_c(out[0])


def build_output_command(output: int, enabled: bool) -> str:
    return build_text(lib.ezo_ec_build_output_command, 32, int(output), int(bool(enabled)))


def build_temperature_command(temperature_c: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_ec_build_temperature_command, 32, float(temperature_c), int(decimals))


def build_probe_k_command(k_value: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_ec_build_probe_k_command, 32, float(k_value), int(decimals))


def build_tds_factor_command(factor: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_ec_build_tds_factor_command, 32, float(factor), int(decimals))


def build_calibration_command(point, reference_value: float, decimals: int = 2) -> str:
    return build_text(lib.ezo_ec_build_calibration_command, 32, int(point), float(reference_value), int(decimals))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(device, *args, timing)))
    return int(timing.wait_ms)


def send_read_i2c(device) -> int:
    return _timed_send(lib.ezo_ec_send_read_i2c, _unwrap_i2c_device(device))


def send_read_with_temp_comp_i2c(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_read_with_temp_comp_i2c, _unwrap_i2c_device(device), float(temperature_c), int(decimals))


def send_output_query_i2c(device) -> int:
    return _timed_send(lib.ezo_ec_send_output_query_i2c, _unwrap_i2c_device(device))


def send_output_set_i2c(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_ec_send_output_set_i2c, _unwrap_i2c_device(device), int(output), int(bool(enabled)))


def send_temperature_query_i2c(device) -> int:
    return _timed_send(lib.ezo_ec_send_temperature_query_i2c, _unwrap_i2c_device(device))


def send_temperature_set_i2c(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_temperature_set_i2c, _unwrap_i2c_device(device), float(temperature_c), int(decimals))


def send_probe_k_query_i2c(device) -> int:
    return _timed_send(lib.ezo_ec_send_probe_k_query_i2c, _unwrap_i2c_device(device))


def send_probe_k_set_i2c(device, k_value: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_probe_k_set_i2c, _unwrap_i2c_device(device), float(k_value), int(decimals))


def send_tds_factor_query_i2c(device) -> int:
    return _timed_send(lib.ezo_ec_send_tds_factor_query_i2c, _unwrap_i2c_device(device))


def send_tds_factor_set_i2c(device, factor: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_tds_factor_set_i2c, _unwrap_i2c_device(device), float(factor), int(decimals))


def send_calibration_query_i2c(device) -> int:
    return _timed_send(lib.ezo_ec_send_calibration_query_i2c, _unwrap_i2c_device(device))


def send_calibration_i2c(device, point, reference_value: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_calibration_i2c, _unwrap_i2c_device(device), int(point), float(reference_value), int(decimals))


def send_clear_calibration_i2c(device) -> int:
    return _timed_send(lib.ezo_ec_send_clear_calibration_i2c, _unwrap_i2c_device(device))


def read_response_i2c(device, enabled_mask: int):
    return read_struct(lib.ezo_ec_read_response_i2c, "ezo_ec_reading_t", ec_reading_from_c, _unwrap_i2c_device(device), int(enabled_mask))


def read_output_config_i2c(device):
    return read_struct(lib.ezo_ec_read_output_config_i2c, "ezo_ec_output_config_t", ec_output_config_from_c, _unwrap_i2c_device(device))


def read_temperature_i2c(device):
    return read_struct(lib.ezo_ec_read_temperature_i2c, "ezo_ec_temperature_compensation_t", ec_temperature_compensation_from_c, _unwrap_i2c_device(device))


def read_probe_k_i2c(device):
    return read_struct(lib.ezo_ec_read_probe_k_i2c, "ezo_ec_probe_k_t", ec_probe_k_from_c, _unwrap_i2c_device(device))


def read_tds_factor_i2c(device):
    return read_struct(lib.ezo_ec_read_tds_factor_i2c, "ezo_ec_tds_factor_t", ec_tds_factor_from_c, _unwrap_i2c_device(device))


def read_calibration_status_i2c(device):
    return read_struct(lib.ezo_ec_read_calibration_status_i2c, "ezo_ec_calibration_status_t", ec_calibration_status_from_c, _unwrap_i2c_device(device))


def send_read_uart(device) -> int:
    return _timed_send(lib.ezo_ec_send_read_uart, _unwrap_uart_device(device))


def send_read_with_temp_comp_uart(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_read_with_temp_comp_uart, _unwrap_uart_device(device), float(temperature_c), int(decimals))


def send_output_query_uart(device) -> int:
    return _timed_send(lib.ezo_ec_send_output_query_uart, _unwrap_uart_device(device))


def send_output_set_uart(device, output: int, enabled: bool) -> int:
    return _timed_send(lib.ezo_ec_send_output_set_uart, _unwrap_uart_device(device), int(output), int(bool(enabled)))


def send_temperature_query_uart(device) -> int:
    return _timed_send(lib.ezo_ec_send_temperature_query_uart, _unwrap_uart_device(device))


def send_temperature_set_uart(device, temperature_c: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_temperature_set_uart, _unwrap_uart_device(device), float(temperature_c), int(decimals))


def send_probe_k_query_uart(device) -> int:
    return _timed_send(lib.ezo_ec_send_probe_k_query_uart, _unwrap_uart_device(device))


def send_probe_k_set_uart(device, k_value: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_probe_k_set_uart, _unwrap_uart_device(device), float(k_value), int(decimals))


def send_tds_factor_query_uart(device) -> int:
    return _timed_send(lib.ezo_ec_send_tds_factor_query_uart, _unwrap_uart_device(device))


def send_tds_factor_set_uart(device, factor: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_tds_factor_set_uart, _unwrap_uart_device(device), float(factor), int(decimals))


def send_calibration_query_uart(device) -> int:
    return _timed_send(lib.ezo_ec_send_calibration_query_uart, _unwrap_uart_device(device))


def send_calibration_uart(device, point, reference_value: float, decimals: int = 2) -> int:
    return _timed_send(lib.ezo_ec_send_calibration_uart, _unwrap_uart_device(device), int(point), float(reference_value), int(decimals))


def send_clear_calibration_uart(device) -> int:
    return _timed_send(lib.ezo_ec_send_clear_calibration_uart, _unwrap_uart_device(device))


def read_response_uart(device, enabled_mask: int):
    return read_struct(lib.ezo_ec_read_response_uart, "ezo_ec_reading_t", ec_reading_from_c, _unwrap_uart_device(device), int(enabled_mask))


def read_output_config_uart(device):
    return read_struct(lib.ezo_ec_read_output_config_uart, "ezo_ec_output_config_t", ec_output_config_from_c, _unwrap_uart_device(device))


def read_temperature_uart(device):
    return read_struct(lib.ezo_ec_read_temperature_uart, "ezo_ec_temperature_compensation_t", ec_temperature_compensation_from_c, _unwrap_uart_device(device))


def read_probe_k_uart(device):
    return read_struct(lib.ezo_ec_read_probe_k_uart, "ezo_ec_probe_k_t", ec_probe_k_from_c, _unwrap_uart_device(device))


def read_tds_factor_uart(device):
    return read_struct(lib.ezo_ec_read_tds_factor_uart, "ezo_ec_tds_factor_t", ec_tds_factor_from_c, _unwrap_uart_device(device))


def read_calibration_status_uart(device):
    return read_struct(lib.ezo_ec_read_calibration_status_uart, "ezo_ec_calibration_status_t", ec_calibration_status_from_c, _unwrap_uart_device(device))
