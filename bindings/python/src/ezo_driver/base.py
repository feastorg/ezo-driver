from __future__ import annotations

from ._call import ascii_bytes, build_text, ffi, lib
from .errors import raise_for_result
from .enums import CommandKind


def get_timing_hint_for_command_kind(kind: CommandKind | int) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(lib.ezo_get_timing_hint_for_command_kind(int(kind), timing)))
    return int(timing.wait_ms)


def result_name(result: int) -> str:
    return ffi.string(lib.ezo_result_name(int(result))).decode("ascii")


def parse_double(text: str) -> float:
    payload = ascii_bytes(text)
    value = ffi.new("double *")
    raise_for_result(int(lib.ezo_parse_double(payload, len(payload), value)))
    return float(value[0])


def format_fixed_command(prefix: str, value: float, decimals: int = 2) -> str:
    return build_text(
        lib.ezo_common_format_fixed_command,
        64,
        ascii_bytes(prefix),
        float(value),
        int(decimals),
    )
