from __future__ import annotations

from ._call import (
    ascii_bytes,
    build_text,
    control_baud_status_from_c,
    control_led_status_from_c,
    control_name_from_c,
    control_protocol_lock_status_from_c,
    control_response_code_status_from_c,
    control_status_from_c,
    device_info_from_c,
    ffi,
    lib,
    read_struct,
)
from ._support import _unwrap_i2c_device, _unwrap_uart_device


def parse_name(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_control_name_t *")
    from .errors import raise_for_result

    raise_for_result(int(lib.ezo_control_parse_name(payload, len(payload), out)))
    return control_name_from_c(out[0])


def parse_status(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_control_status_t *")
    from .errors import raise_for_result

    raise_for_result(int(lib.ezo_control_parse_status(payload, len(payload), out)))
    return control_status_from_c(out[0])


def parse_led(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_control_led_status_t *")
    from .errors import raise_for_result

    raise_for_result(int(lib.ezo_control_parse_led(payload, len(payload), out)))
    return control_led_status_from_c(out[0])


def parse_protocol_lock(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_control_protocol_lock_status_t *")
    from .errors import raise_for_result

    raise_for_result(int(lib.ezo_control_parse_protocol_lock(payload, len(payload), out)))
    return control_protocol_lock_status_from_c(out[0])


def parse_baud(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_control_baud_status_t *")
    from .errors import raise_for_result

    raise_for_result(int(lib.ezo_control_parse_baud(payload, len(payload), out)))
    return control_baud_status_from_c(out[0])


def parse_response_code(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_control_response_code_status_t *")
    from .errors import raise_for_result

    raise_for_result(int(lib.ezo_control_parse_response_code(payload, len(payload), out)))
    return control_response_code_status_from_c(out[0])


def build_name_command(name: str) -> str:
    return build_text(lib.ezo_control_build_name_command, 64, ascii_bytes(name))


def build_led_command(enabled: bool) -> str:
    return build_text(lib.ezo_control_build_led_command, 32, int(bool(enabled)))


def build_protocol_lock_command(enabled: bool) -> str:
    return build_text(lib.ezo_control_build_protocol_lock_command, 32, int(bool(enabled)))


def build_switch_to_i2c_command(i2c_address: int) -> str:
    return build_text(lib.ezo_control_build_switch_to_i2c_command, 32, int(i2c_address))


def build_switch_to_uart_command(baud_rate: int) -> str:
    return build_text(lib.ezo_control_build_switch_to_uart_command, 32, int(baud_rate))


def build_response_code_command(enabled: bool) -> str:
    return build_text(lib.ezo_control_build_response_code_command, 32, int(bool(enabled)))


def _timed_send(func, device, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    from .errors import raise_for_result

    raise_for_result(int(func(device, *args, timing)))
    return int(timing.wait_ms)


def _read_i2c(func, ctype: str, converter, device):
    return read_struct(func, ctype, converter, _unwrap_i2c_device(device))


def _read_uart(func, ctype: str, converter, device):
    return read_struct(func, ctype, converter, _unwrap_uart_device(device))


def send_info_query_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_info_query_i2c, _unwrap_i2c_device(device), int(product_id))


def send_name_query_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_name_query_i2c, _unwrap_i2c_device(device), int(product_id))


def send_name_set_i2c(device, product_id, name: str) -> int:
    return _timed_send(
        lib.ezo_control_send_name_set_i2c,
        _unwrap_i2c_device(device),
        int(product_id),
        ascii_bytes(name),
    )


def send_status_query_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_status_query_i2c, _unwrap_i2c_device(device), int(product_id))


def send_led_query_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_led_query_i2c, _unwrap_i2c_device(device), int(product_id))


def send_led_set_i2c(device, product_id, enabled: bool) -> int:
    return _timed_send(lib.ezo_control_send_led_set_i2c, _unwrap_i2c_device(device), int(product_id), int(bool(enabled)))


def send_find_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_find_i2c, _unwrap_i2c_device(device), int(product_id))


def send_sleep_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_sleep_i2c, _unwrap_i2c_device(device), int(product_id))


def send_factory_reset_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_factory_reset_i2c, _unwrap_i2c_device(device), int(product_id))


def send_protocol_lock_query_i2c(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_protocol_lock_query_i2c, _unwrap_i2c_device(device), int(product_id))


def send_protocol_lock_set_i2c(device, product_id, enabled: bool) -> int:
    return _timed_send(
        lib.ezo_control_send_protocol_lock_set_i2c,
        _unwrap_i2c_device(device),
        int(product_id),
        int(bool(enabled)),
    )


def send_switch_to_uart_i2c(device, product_id, baud_rate: int) -> int:
    return _timed_send(lib.ezo_control_send_switch_to_uart_i2c, _unwrap_i2c_device(device), int(product_id), int(baud_rate))


def read_info_i2c(device):
    return _read_i2c(lib.ezo_control_read_info_i2c, "ezo_device_info_t", device_info_from_c, device)


def read_name_i2c(device):
    return _read_i2c(lib.ezo_control_read_name_i2c, "ezo_control_name_t", control_name_from_c, device)


def read_status_i2c(device):
    return _read_i2c(lib.ezo_control_read_status_i2c, "ezo_control_status_t", control_status_from_c, device)


def read_led_i2c(device):
    return _read_i2c(lib.ezo_control_read_led_i2c, "ezo_control_led_status_t", control_led_status_from_c, device)


def read_protocol_lock_i2c(device):
    return _read_i2c(
        lib.ezo_control_read_protocol_lock_i2c,
        "ezo_control_protocol_lock_status_t",
        control_protocol_lock_status_from_c,
        device,
    )


def send_info_query_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_info_query_uart, _unwrap_uart_device(device), int(product_id))


def send_name_query_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_name_query_uart, _unwrap_uart_device(device), int(product_id))


def send_name_set_uart(device, product_id, name: str) -> int:
    return _timed_send(
        lib.ezo_control_send_name_set_uart,
        _unwrap_uart_device(device),
        int(product_id),
        ascii_bytes(name),
    )


def send_status_query_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_status_query_uart, _unwrap_uart_device(device), int(product_id))


def send_led_query_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_led_query_uart, _unwrap_uart_device(device), int(product_id))


def send_led_set_uart(device, product_id, enabled: bool) -> int:
    return _timed_send(lib.ezo_control_send_led_set_uart, _unwrap_uart_device(device), int(product_id), int(bool(enabled)))


def send_find_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_find_uart, _unwrap_uart_device(device), int(product_id))


def send_sleep_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_sleep_uart, _unwrap_uart_device(device), int(product_id))


def send_factory_reset_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_factory_reset_uart, _unwrap_uart_device(device), int(product_id))


def send_protocol_lock_query_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_protocol_lock_query_uart, _unwrap_uart_device(device), int(product_id))


def send_protocol_lock_set_uart(device, product_id, enabled: bool) -> int:
    return _timed_send(
        lib.ezo_control_send_protocol_lock_set_uart,
        _unwrap_uart_device(device),
        int(product_id),
        int(bool(enabled)),
    )


def send_baud_query_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_baud_query_uart, _unwrap_uart_device(device), int(product_id))


def send_response_code_query_uart(device, product_id) -> int:
    return _timed_send(lib.ezo_control_send_response_code_query_uart, _unwrap_uart_device(device), int(product_id))


def send_response_code_set_uart(device, product_id, enabled: bool) -> int:
    return _timed_send(
        lib.ezo_control_send_response_code_set_uart,
        _unwrap_uart_device(device),
        int(product_id),
        int(bool(enabled)),
    )


def send_switch_to_i2c_uart(device, product_id, i2c_address: int) -> int:
    return _timed_send(lib.ezo_control_send_switch_to_i2c_uart, _unwrap_uart_device(device), int(product_id), int(i2c_address))


def send_switch_to_uart_uart(device, product_id, baud_rate: int) -> int:
    return _timed_send(lib.ezo_control_send_switch_to_uart_uart, _unwrap_uart_device(device), int(product_id), int(baud_rate))


def read_info_uart(device):
    return _read_uart(lib.ezo_control_read_info_uart, "ezo_device_info_t", device_info_from_c, device)


def read_name_uart(device):
    return _read_uart(lib.ezo_control_read_name_uart, "ezo_control_name_t", control_name_from_c, device)


def read_status_uart(device):
    return _read_uart(lib.ezo_control_read_status_uart, "ezo_control_status_t", control_status_from_c, device)


def read_led_uart(device):
    return _read_uart(lib.ezo_control_read_led_uart, "ezo_control_led_status_t", control_led_status_from_c, device)


def read_protocol_lock_uart(device):
    return _read_uart(
        lib.ezo_control_read_protocol_lock_uart,
        "ezo_control_protocol_lock_status_t",
        control_protocol_lock_status_from_c,
        device,
    )


def read_baud_uart(device):
    return _read_uart(lib.ezo_control_read_baud_uart, "ezo_control_baud_status_t", control_baud_status_from_c, device)


def read_response_code_uart(device):
    return _read_uart(
        lib.ezo_control_read_response_code_uart,
        "ezo_control_response_code_status_t",
        control_response_code_status_from_c,
        device,
    )
