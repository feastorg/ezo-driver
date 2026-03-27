from __future__ import annotations

from ._call import (
    ascii_bytes,
    ffi,
    lib,
    multi_output_reading_from_c,
    output_schema_from_c,
    scalar_reading_from_c,
)
from .errors import raise_for_result


def get_output_schema(product_id):
    out = ffi.new("ezo_output_schema_t *")
    raise_for_result(int(lib.ezo_schema_get_output_schema(int(product_id), out)))
    return output_schema_from_c(out[0])


def count_enabled_fields(schema, enabled_mask: int) -> int:
    c_schema = ffi.new("ezo_output_schema_t *")
    c_schema.product_id = int(schema.product_id)
    c_schema.field_count = len(schema.fields)
    for index, field in enumerate(schema.fields):
        c_schema.fields[index] = int(field)
    return int(lib.ezo_schema_count_enabled_fields(c_schema, int(enabled_mask)))


def parse_scalar_reading(text: str, field):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_scalar_reading_t *")
    raise_for_result(int(lib.ezo_schema_parse_scalar_reading(payload, len(payload), int(field), out)))
    return scalar_reading_from_c(out[0])


def parse_multi_output_reading(text: str, schema, enabled_mask: int):
    payload = ascii_bytes(text)
    c_schema = ffi.new("ezo_output_schema_t *")
    c_schema.product_id = int(schema.product_id)
    c_schema.field_count = len(schema.fields)
    for index, field in enumerate(schema.fields):
        c_schema.fields[index] = int(field)
    out = ffi.new("ezo_multi_output_reading_t *")
    raise_for_result(
        int(
            lib.ezo_schema_parse_multi_output_reading(
                payload,
                len(payload),
                c_schema,
                int(enabled_mask),
                out,
            )
        )
    )
    return multi_output_reading_from_c(out[0])
