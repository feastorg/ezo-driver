from __future__ import annotations

import pytest

from ezo_driver import ProductId, ProductSupport, product


@pytest.mark.parametrize(
    ("reply", "product_id", "address", "tier"),
    [
        ("?i,CO2,1.0", ProductId.CO2, 105, ProductSupport.METADATA),
        ("?i,O2,1.00", ProductId.O2, 108, ProductSupport.TYPED_READ),
        ("?i,RGB,2.1", ProductId.RGB, 112, ProductSupport.METADATA),
        ("?i,PRS, 1.0", ProductId.PRS, 106, ProductSupport.METADATA),
        ("?i,FLO,2.00", ProductId.FLO, 104, ProductSupport.METADATA),
        ("?i,PMP, 1.1", ProductId.PMP, 103, ProductSupport.METADATA),
        ("?i,PMPL, 1.1", ProductId.PMP_L, 109, ProductSupport.METADATA),
    ],
)
def test_new_circuits_are_identified(reply, product_id, address, tier):
    info = product.parse_device_info(reply)
    assert info.product_id is product_id

    meta = product.get_metadata(product_id)
    assert meta is not None
    assert meta.default_i2c_address == address
    assert meta.support_tier is tier
