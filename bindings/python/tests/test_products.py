from __future__ import annotations

import pytest

from ezo_driver import DeviceStatus
from ezo_driver import do, ec, hum, orp, ph, rtd
from ezo_driver.enums import (
    DOCalibrationPoint,
    DOOutputMask,
    DOSalinityUnit,
    ECOutputMask,
    ECCalibrationPoint,
    HUMOutputMask,
    ORPExtendedScale,
    PHCalibrationPoint,
    PHExtendedRange,
    RTDScale,
)

from ._support import FakeI2CDevice, FakeUARTDevice, i2c_response, uart_lines


def test_ph_helpers_cover_parse_build_i2c_and_uart():
    assert ph.parse_reading("7.156").ph == pytest.approx(7.156)
    assert ph.build_temperature_command(19.5, 2) == "T,19.50"
    assert ph.build_calibration_command(PHCalibrationPoint.HIGH, 10.0, 2) == "Cal,high,10.00"

    i2c_dev = FakeI2CDevice()
    wait_ms = ph.send_read_with_temp_comp_i2c(i2c_dev, 19.5, 3)
    assert wait_ms == 900
    assert i2c_dev.last_tx == b"rt,19.500"

    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "7.15"))
    reading = ph.read_response_i2c(i2c_dev)
    assert reading.ph == pytest.approx(7.15)

    uart_dev = FakeUARTDevice()
    wait_ms = ph.send_temperature_set_uart(uart_dev, 19.5, 1)
    assert wait_ms == 300
    assert uart_dev.tx_bytes == b"T,19.5\r"

    uart_dev.set_response(uart_lines("?pHext,1", "*OK"))
    wait_ms = ph.send_extended_range_query_uart(uart_dev)
    assert wait_ms == 300
    extended = ph.read_extended_range_uart(uart_dev)
    assert extended.enabled is PHExtendedRange.ENABLED


def test_orp_helpers_cover_parse_build_i2c_and_uart():
    assert orp.parse_reading("225.4").millivolts == pytest.approx(225.4)
    assert orp.build_calibration_command(225.0, 0) == "Cal,225"
    assert orp.build_extended_scale_command(ORPExtendedScale.ENABLED) == "ORPext,1"

    i2c_dev = FakeI2CDevice(address=98)
    wait_ms = orp.send_calibration_i2c(i2c_dev, 225.0, 0)
    assert wait_ms == 900
    assert i2c_dev.last_tx == b"Cal,225"

    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "?Cal,1"))
    wait_ms = orp.send_calibration_query_i2c(i2c_dev)
    assert wait_ms == 300
    calibration = orp.read_calibration_status_i2c(i2c_dev)
    assert calibration.calibrated is True

    uart_dev = FakeUARTDevice()
    uart_dev.set_response(uart_lines("225.4", "*OK"))
    wait_ms = orp.send_read_uart(uart_dev)
    assert wait_ms == 1000
    reading = orp.read_response_uart(uart_dev)
    assert reading.millivolts == pytest.approx(225.4)


def test_rtd_helpers_cover_parse_build_i2c_and_uart():
    memory_values = rtd.parse_memory_all("100.00,104.00,108.00,112.00", RTDScale.FAHRENHEIT)
    assert len(memory_values) == 4
    assert memory_values[-1].temperature == pytest.approx(112.0)
    assert rtd.build_scale_command(RTDScale.FAHRENHEIT) == "S,f"

    i2c_dev = FakeI2CDevice(address=102)
    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "100.00,104.00,108.00,112.00"))
    wait_ms = rtd.send_memory_all_i2c(i2c_dev)
    assert wait_ms == 300
    values = rtd.read_memory_all_i2c(i2c_dev, RTDScale.FAHRENHEIT)
    assert [round(value.temperature, 2) for value in values] == [100.0, 104.0, 108.0, 112.0]

    uart_dev = FakeUARTDevice()
    uart_dev.set_response(uart_lines("?S,c", "*OK"))
    wait_ms = rtd.send_scale_query_uart(uart_dev)
    assert wait_ms == 300
    scale = rtd.read_scale_uart(uart_dev)
    assert scale.scale is RTDScale.CELSIUS


def test_ec_helpers_cover_parse_build_i2c_and_uart():
    reading = ec.parse_reading(
        "412.0,1.025",
        int(ECOutputMask.CONDUCTIVITY | ECOutputMask.SPECIFIC_GRAVITY | 0x80),
    )
    assert reading.present_mask == (ECOutputMask.CONDUCTIVITY | ECOutputMask.SPECIFIC_GRAVITY)
    assert ec.build_output_command(ECOutputMask.SALINITY, False) == "O,S,0"
    assert ec.build_calibration_command(ECCalibrationPoint.DRY, 0.0, 0) == "Cal,dry"

    i2c_dev = FakeI2CDevice(address=100)
    wait_ms = ec.send_read_with_temp_comp_i2c(i2c_dev, 19.5, 3)
    assert wait_ms == 900
    assert i2c_dev.last_tx == b"rt,19.500"

    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "842,1.021"))
    reading = ec.read_response_i2c(
        i2c_dev,
        int(ECOutputMask.CONDUCTIVITY | ECOutputMask.SPECIFIC_GRAVITY),
    )
    assert reading.conductivity_us_cm == pytest.approx(842.0)
    assert reading.specific_gravity == pytest.approx(1.021)

    uart_dev = FakeUARTDevice()
    uart_dev.set_response(uart_lines("?,O,EC,S", "*OK"))
    wait_ms = ec.send_output_query_uart(uart_dev)
    assert wait_ms == 300
    output = ec.read_output_config_uart(uart_dev)
    assert output.enabled_mask == (ECOutputMask.CONDUCTIVITY | ECOutputMask.SALINITY)


def test_do_helpers_cover_parse_build_i2c_and_uart():
    salinity = do.parse_salinity("?S,12.5,ppt")
    assert salinity.value == pytest.approx(12.5)
    assert salinity.unit is DOSalinityUnit.PPT
    assert do.build_salinity_command(1200.0, DOSalinityUnit.MICROSIEMENS, 1) == "S,1200.0,uS"
    assert do.build_calibration_command(DOCalibrationPoint.ZERO) == "Cal,0"

    i2c_dev = FakeI2CDevice(address=97)
    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "8.50,94.2"))
    reading = do.read_response_i2c(
        i2c_dev,
        int(DOOutputMask.MG_L | DOOutputMask.PERCENT_SATURATION),
    )
    assert reading.milligrams_per_liter == pytest.approx(8.50)
    assert reading.percent_saturation == pytest.approx(94.2)

    uart_dev = FakeUARTDevice()
    wait_ms = do.send_salinity_set_uart(uart_dev, 35.0, DOSalinityUnit.PPT, 1)
    assert wait_ms == 300
    assert uart_dev.tx_bytes == b"S,35.0,ppt\r"

    uart_dev.set_response(uart_lines("?,P,99.8", "*OK"))
    wait_ms = do.send_pressure_query_uart(uart_dev)
    assert wait_ms == 300
    pressure = do.read_pressure_uart(uart_dev)
    assert pressure.pressure_kpa == pytest.approx(99.8)


def test_hum_helpers_cover_parse_build_i2c_and_uart():
    reading = hum.parse_reading(
        "48.5,9.2",
        int(HUMOutputMask.HUMIDITY | HUMOutputMask.DEW_POINT | 0x80),
    )
    assert reading.present_mask == (HUMOutputMask.HUMIDITY | HUMOutputMask.DEW_POINT)
    assert hum.build_output_command(HUMOutputMask.AIR_TEMPERATURE, False) == "O,T,0"
    assert hum.build_temperature_calibration_command(25.7, 1) == "Tcal,25.7"

    i2c_dev = FakeI2CDevice(address=111)
    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "44.0,21.8,8.1"))
    reading = hum.read_response_i2c(
        i2c_dev,
        int(HUMOutputMask.HUMIDITY | HUMOutputMask.AIR_TEMPERATURE | HUMOutputMask.DEW_POINT),
    )
    assert reading.relative_humidity_percent == pytest.approx(44.0)
    assert reading.air_temperature_c == pytest.approx(21.8)
    assert reading.dew_point_c == pytest.approx(8.1)

    uart_dev = FakeUARTDevice()
    wait_ms = hum.send_output_set_uart(uart_dev, HUMOutputMask.HUMIDITY, False)
    assert wait_ms == 300
    assert uart_dev.tx_bytes == b"O,HUM,0\r"

    uart_dev.set_response(uart_lines("?Tcal,1", "*OK"))
    wait_ms = hum.send_temperature_calibration_query_uart(uart_dev)
    assert wait_ms == 300
    calibration = hum.read_temperature_calibration_status_uart(uart_dev)
    assert calibration.calibrated is True
