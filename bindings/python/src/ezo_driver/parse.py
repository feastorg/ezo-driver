from __future__ import annotations

from ._call import ascii_bytes, ffi, lib
from .errors import raise_for_result
from .enums import UARTResponseKind, UARTSequenceStep


def _span_to_text(span) -> str:
    return ffi.string(span.text, int(span.length)).decode("ascii")


def text_span_is_empty(text: str) -> bool:
    return len(text.strip()) == 0


def text_span_equals(text: str, expected: str) -> bool:
    return text.strip() == expected


def parse_text_span_uint32(text: str) -> int:
    payload = ascii_bytes(text)
    c_text = ffi.new("char[]", payload)
    span = ffi.new("ezo_text_span_t *")
    span.text = c_text
    span.length = len(payload)
    out = ffi.new("uint32_t *")
    raise_for_result(int(lib.ezo_parse_text_span_uint32(span[0], out)))
    return int(out[0])


def parse_text_span_double(text: str) -> float:
    payload = ascii_bytes(text)
    c_text = ffi.new("char[]", payload)
    span = ffi.new("ezo_text_span_t *")
    span.text = c_text
    span.length = len(payload)
    out = ffi.new("double *")
    raise_for_result(int(lib.ezo_parse_text_span_double(span[0], out)))
    return float(out[0])


def parse_csv_fields(text: str) -> list[str]:
    payload = ascii_bytes(text)
    fields = ffi.new("ezo_text_span_t[8]")
    count = ffi.new("size_t *")
    raise_for_result(int(lib.ezo_parse_csv_fields(payload, len(payload), fields, 8, count)))
    return [_span_to_text(fields[i]) for i in range(int(count[0]))]


def parse_query_response(text: str) -> tuple[str, list[str]]:
    payload = ascii_bytes(text)
    prefix = ffi.new("ezo_text_span_t *")
    fields = ffi.new("ezo_text_span_t[8]")
    count = ffi.new("size_t *")
    raise_for_result(int(lib.ezo_parse_query_response(payload, len(payload), prefix, fields, 8, count)))
    return _span_to_text(prefix[0]), [_span_to_text(fields[i]) for i in range(int(count[0]))]


def parse_prefixed_fields(text: str, prefix: str) -> list[str]:
    payload = ascii_bytes(text)
    prefix_bytes = ascii_bytes(prefix)
    fields = ffi.new("ezo_text_span_t[8]")
    count = ffi.new("size_t *")
    raise_for_result(
        int(lib.ezo_parse_prefixed_fields(payload, len(payload), prefix_bytes, fields, 8, count))
    )
    return [_span_to_text(fields[i]) for i in range(int(count[0]))]


class UARTSequence:
    def __init__(self):
        self._sequence = ffi.new("ezo_uart_sequence_t *")
        lib.ezo_uart_sequence_init(self._sequence)

    def reset(self) -> None:
        lib.ezo_uart_sequence_init(self._sequence)

    def push_line(self, kind: UARTResponseKind | int) -> UARTSequenceStep:
        step = ffi.new("ezo_uart_sequence_step_t *")
        raise_for_result(int(lib.ezo_uart_sequence_push_line(self._sequence, int(kind), step)))
        return UARTSequenceStep(int(step[0]))

    def is_complete(self) -> bool:
        return bool(lib.ezo_uart_sequence_is_complete(self._sequence))

    @property
    def line_count(self) -> int:
        return int(self._sequence.line_count)

    @property
    def data_line_count(self) -> int:
        return int(self._sequence.data_line_count)

    @property
    def control_line_count(self) -> int:
        return int(self._sequence.control_line_count)

    @property
    def last_kind(self) -> UARTResponseKind:
        return UARTResponseKind(int(self._sequence.last_kind))

    @property
    def terminal_kind(self) -> UARTResponseKind:
        return UARTResponseKind(int(self._sequence.terminal_kind))

    @property
    def complete(self) -> bool:
        return bool(self._sequence.complete)
