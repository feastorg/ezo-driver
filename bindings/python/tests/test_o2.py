from __future__ import annotations

import pytest

from ezo_driver import DeviceStatus, EzoParseError, EzoProtocolError, ProductId, o2, product
from ezo_driver.enums import O2OutputMask, ProductSupport

from ._support import FakeI2CDevice, FakeUARTDevice, i2c_response, uart_lines

BOTH = O2OutputMask.PERCENT | O2OutputMask.PPT


def test_parse_reading_follows_the_enabled_outputs():
    single = o2.parse_reading("20.95", O2OutputMask.PERCENT)
    assert single.present_mask == O2OutputMask.PERCENT
    assert single.percent == pytest.approx(20.95)

    # Percent first is inferred from `?,O,%,PPT`; no datasheet page prints a two-value reading.
    both = o2.parse_reading("20.95,209.5", BOTH)
    assert both.percent == pytest.approx(20.95)
    assert both.parts_per_thousand == pytest.approx(209.5)

    with pytest.raises(EzoParseError):
        o2.parse_reading("20.95", BOTH)


def test_builders():
    assert o2.build_output_command(O2OutputMask.PPT, True) == "O,PPT,1"
    assert o2.build_output_command(O2OutputMask.PERCENT, False) == "O,%,0"
    assert o2.build_temperature_command(19.5, 1) == "T,19.5"


def test_i2c_helpers():
    device = FakeI2CDevice(address=108)

    assert o2.send_read_i2c(device) == 900
    assert device.last_tx == b"r"
    device.set_response(i2c_response(DeviceStatus.SUCCESS, "20.95"))
    assert o2.read_response_i2c(device, O2OutputMask.PERCENT).percent == pytest.approx(20.95)

    assert o2.send_read_with_temp_comp_i2c(device, 19.5, 1) == 900
    assert device.last_tx == b"rt,19.5"

    assert o2.send_temperature_set_i2c(device, 19.5, 1) == 300
    assert device.last_tx == b"T,19.5"

    assert o2.send_temperature_query_i2c(device) == 300
    device.set_response(i2c_response(DeviceStatus.SUCCESS, "?T,19.5"))
    assert o2.read_temperature_i2c(device).temperature_c == pytest.approx(19.5)

    assert o2.send_output_set_i2c(device, O2OutputMask.PPT, True) == 300
    assert device.last_tx == b"O,PPT,1"

    assert o2.send_output_query_i2c(device) == 300
    device.set_response(i2c_response(DeviceStatus.SUCCESS, "?,O,%,PPT"))
    assert o2.read_output_config_i2c(device).enabled_mask == BOTH


def test_uart_helpers():
    device = FakeUARTDevice()

    device.set_response(uart_lines("20.95", "*OK"))
    assert o2.send_read_uart(device) == 900
    assert o2.read_response_uart(device, O2OutputMask.PERCENT).percent == pytest.approx(20.95)

    device.set_response(uart_lines("?T,19.5", "*OK"))
    assert o2.send_temperature_query_uart(device) == 300
    assert o2.read_temperature_uart(device).temperature_c == pytest.approx(19.5)

    device.set_response(uart_lines("?,O,%", "*OK"))
    assert o2.send_output_query_uart(device) == 300
    assert o2.read_output_config_uart(device).enabled_mask == O2OutputMask.PERCENT

    assert o2.send_output_set_uart(device, O2OutputMask.PERCENT, False) == 300
    assert o2.send_temperature_set_uart(device, 19.5, 1) == 300


def test_uart_read_with_temp_comp_expects_ok_before_the_value():
    device = FakeUARTDevice()

    device.set_response(uart_lines("*OK", "20.95"))
    assert o2.send_read_with_temp_comp_uart(device, 19.5, 1) == 900
    reading = o2.read_response_with_temp_comp_uart(device, O2OutputMask.PERCENT)
    assert reading.percent == pytest.approx(20.95)

    device.set_response(uart_lines("20.95", "*OK"))
    with pytest.raises(EzoProtocolError):
        o2.read_response_with_temp_comp_uart(device, O2OutputMask.PERCENT)


def test_o2_is_typed_read_tier():
    assert product.get_support_tier(ProductId.O2) is ProductSupport.TYPED_READ
