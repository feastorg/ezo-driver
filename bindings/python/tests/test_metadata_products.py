from __future__ import annotations

import pytest

from ezo_driver import ProductId, ProductSupport, product


@pytest.mark.parametrize(
    ("reply", "product_id", "address"),
    [
        ("?i,CO2,1.0", ProductId.CO2, 105),
        ("?i,O2,1.00", ProductId.O2, 108),
        ("?i,RGB,2.1", ProductId.RGB, 112),
        ("?i,PRS, 1.0", ProductId.PRS, 106),
        ("?i,FLO,2.00", ProductId.FLO, 104),
        ("?i,PMP, 1.1", ProductId.PMP, 103),
        ("?i,PMPL, 1.1", ProductId.PMP_L, 109),
    ],
)
def test_metadata_tier_products_are_identified(reply, product_id, address):
    info = product.parse_device_info(reply)
    assert info.product_id is product_id

    meta = product.get_metadata(product_id)
    assert meta is not None
    assert meta.default_i2c_address == address
    assert meta.support_tier is ProductSupport.METADATA
