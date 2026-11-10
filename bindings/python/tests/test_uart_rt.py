from __future__ import annotations

import pytest

from ezo_driver import EzoProtocolError, do, ec

from ._support import FakeUARTDevice, uart_lines


@pytest.mark.parametrize(
    ("module", "enabled_mask", "field"),
    [
        (ec, 0x1, "conductivity_us_cm"),
        (do, 0x1, "milligrams_per_liter"),
    ],
)
def test_read_with_temp_comp_uart_expects_ok_before_the_value(module, enabled_mask, field):
    device = FakeUARTDevice()

    device.set_response(uart_lines("*OK", "8.91"))
    assert module.send_read_with_temp_comp_uart(device, 19.5, 1) == 900
    reading = module.read_response_with_temp_comp_uart(device, enabled_mask)
    assert getattr(reading, field) == pytest.approx(8.91)

    device.set_response(uart_lines("8.91", "*OK"))
    with pytest.raises(EzoProtocolError):
        module.read_response_with_temp_comp_uart(device, enabled_mask)
