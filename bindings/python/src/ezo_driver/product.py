from __future__ import annotations

from ._call import ascii_bytes, device_info_from_c, ffi, lib, product_metadata_from_c
from .errors import raise_for_result
from .enums import ProductId, ProductSupport


def product_id_from_short_code(short_code: str):
    payload = ascii_bytes(short_code)
    out = ffi.new("ezo_product_id_t *")
    raise_for_result(int(lib.ezo_product_id_from_short_code(payload, len(payload), out)))
    return ProductId(int(out[0]))


def parse_device_info(text: str):
    payload = ascii_bytes(text)
    out = ffi.new("ezo_device_info_t *")
    raise_for_result(int(lib.ezo_parse_device_info(payload, len(payload), out)))
    return device_info_from_c(out[0])


def get_metadata(product_id):
    return product_metadata_from_c(lib.ezo_product_get_metadata(int(product_id)))


def get_metadata_by_short_code(short_code: str):
    payload = ascii_bytes(short_code)
    return product_metadata_from_c(lib.ezo_product_get_metadata_by_short_code(payload, len(payload)))


def get_timing_hint(product_id, transport, kind) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(lib.ezo_product_get_timing_hint(int(product_id), int(transport), int(kind), timing)))
    return int(timing.wait_ms)


def resolve_timing_hint(product_id, transport, kind) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(
        int(lib.ezo_product_resolve_timing_hint(int(product_id), int(transport), int(kind), timing))
    )
    return int(timing.wait_ms)


def get_support_tier(product_id):
    return ProductSupport(int(lib.ezo_product_get_support_tier(int(product_id))))


def supports_capability(product_id, capability_flag) -> bool:
    return bool(lib.ezo_product_supports_capability(int(product_id), int(capability_flag)))


def has_command_family(product_id, family_flag) -> bool:
    return bool(lib.ezo_product_has_command_family(int(product_id), int(family_flag)))
