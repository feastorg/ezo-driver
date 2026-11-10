from __future__ import annotations

import pytest

from ezo_driver import DeviceStatus, EzoParseError, ProductId, co2, product
from ezo_driver.enums import CO2OutputMask, ProductSupport

from ._support import FakeI2CDevice, FakeUARTDevice, i2c_response, uart_lines

BOTH = CO2OutputMask.PPM | CO2OutputMask.INTERNAL_TEMPERATURE


def test_parse_reading_follows_the_enabled_outputs():
    single = co2.parse_reading("6500", CO2OutputMask.PPM)
    assert single.present_mask == CO2OutputMask.PPM
    assert single.ppm == pytest.approx(6500)
    assert single.warming is False

    # ppm first is inferred from `?O,ppm,t`; no datasheet page prints a two-value reading.
    both = co2.parse_reading("800,25.1", BOTH)
    assert both.ppm == pytest.approx(800)
    assert both.internal_temperature_c == pytest.approx(25.1)

    with pytest.raises(EzoParseError):
        co2.parse_reading("6500", BOTH)


def test_parse_reading_reports_warm_up():
    reading = co2.parse_reading("*warm", CO2OutputMask.PPM)
    assert reading.warming is True
    assert reading.present_mask == CO2OutputMask(0)


def test_builder_only_toggles_temperature():
    assert co2.build_output_command(CO2OutputMask.INTERNAL_TEMPERATURE, True) == "O,t,1"


def test_i2c_helpers():
    device = FakeI2CDevice(address=105)

    assert co2.send_read_i2c(device) == 900
    assert device.last_tx == b"r"
    device.set_response(i2c_response(DeviceStatus.SUCCESS, "6500"))
    assert co2.read_response_i2c(device, CO2OutputMask.PPM).ppm == pytest.approx(6500)

    assert co2.send_output_set_i2c(device, CO2OutputMask.INTERNAL_TEMPERATURE, True) == 300
    assert device.last_tx == b"O,t,1"

    assert co2.send_output_query_i2c(device) == 300
    device.set_response(i2c_response(DeviceStatus.SUCCESS, "?O,ppm,t"))
    assert co2.read_output_config_i2c(device).enabled_mask == BOTH


def test_uart_helpers():
    device = FakeUARTDevice()

    device.set_response(uart_lines("6500", "*OK"))
    assert co2.send_read_uart(device) == 900
    assert co2.read_response_uart(device, CO2OutputMask.PPM).ppm == pytest.approx(6500)

    device.set_response(uart_lines("?O,ppm", "*OK"))
    assert co2.send_output_query_uart(device) == 300
    assert co2.read_output_config_uart(device).enabled_mask == CO2OutputMask.PPM

    assert co2.send_output_set_uart(device, CO2OutputMask.INTERNAL_TEMPERATURE, False) == 300


def test_co2_is_typed_read_tier():
    assert product.get_support_tier(ProductId.CO2) is ProductSupport.TYPED_READ
