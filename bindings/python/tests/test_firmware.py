from __future__ import annotations

import pytest

import ezo_driver
from ezo_driver import (
    DeviceStatus,
    EzoParseError,
    FirmwareFeature,
    FirmwareIssue,
    FirmwareVersion,
    ProductId,
    control,
    product,
)

from ._support import FakeI2CDevice, i2c_response


@pytest.mark.parametrize("name", ["FirmwareVersion", "FirmwareFeature", "FirmwareIssue"])
def test_firmware_types_are_public(name):
    assert name in ezo_driver.__all__


@pytest.mark.parametrize(
    ("text", "expected"),
    [
        ("2.16", FirmwareVersion(2, 16)),
        ("1.1", FirmwareVersion(1, 10)),
        ("1.10", FirmwareVersion(1, 10)),
        ("1.01", FirmwareVersion(1, 1)),
        ("1.0", FirmwareVersion(1, 0)),
    ],
)
def test_parse_firmware_version(text, expected):
    assert product.parse_firmware_version(text) == expected


@pytest.mark.parametrize("text", ["", "2", "2.", ".5", "2.x", "2.123", " 2.1"])
def test_parse_firmware_version_rejects_other_text(text):
    with pytest.raises(EzoParseError):
        product.parse_firmware_version(text)


def test_firmware_versions_order_as_decimals():
    parse = product.parse_firmware_version
    assert parse("1.05") < parse("1.1")
    assert parse("1.7") < parse("1.95")
    assert parse("2.01") < parse("2.10")
    assert parse("1.98") < parse("2.10")


def test_device_info_carries_a_comparable_firmware():
    info = product.parse_device_info("?i,pH,2.16")
    assert info.firmware_version == "2.16"
    assert info.firmware == FirmwareVersion(2, 16)


def test_device_info_firmware_is_none_for_other_version_text():
    info = product.parse_device_info("?i,pH,abc")
    assert info.firmware_version == "abc"
    assert info.firmware is None


@pytest.mark.parametrize(
    ("reply", "product_id", "feature", "expected"),
    [
        ("?i,pH,2.13", ProductId.PH, FirmwareFeature.EXTENDED_RANGE, False),
        ("?i,pH,2.14", ProductId.PH, FirmwareFeature.EXTENDED_RANGE, True),
        ("?i,RTD,1.01", ProductId.RTD, FirmwareFeature.PROTOCOL_LOCK, False),
        ("?i,RTD,1.02", ProductId.RTD, FirmwareFeature.PROTOCOL_LOCK, True),
        ("?i,EC,2.12", ProductId.EC, FirmwareFeature.READ_WITH_TEMP_COMP, False),
        ("?i,EC,2.13", ProductId.EC, FirmwareFeature.READ_WITH_TEMP_COMP, True),
        ("?i,HUM,1.0", ProductId.HUM, FirmwareFeature.TEMPERATURE_CALIBRATION, False),
        ("?i,HUM,1.1", ProductId.HUM, FirmwareFeature.TEMPERATURE_CALIBRATION, True),
        ("?i,ORP,9.99", ProductId.ORP, FirmwareFeature.READ_WITH_TEMP_COMP, False),
    ],
)
def test_feature_lookup_from_a_device_reply(reply, product_id, feature, expected):
    device = FakeI2CDevice(address=99)
    device.set_response(i2c_response(DeviceStatus.SUCCESS, reply))
    info = control.read_info_i2c(device)
    assert info.product_id is product_id
    assert product.firmware_has_feature(info.product_id, feature, info.firmware) is expected


@pytest.mark.parametrize(
    ("reply", "product_id", "issue", "expected"),
    [
        ("?i,pH,2.15", ProductId.PH, FirmwareIssue.I2C_SLEEP_WAKE, True),
        ("?i,pH,2.16", ProductId.PH, FirmwareIssue.I2C_SLEEP_WAKE, False),
        ("?i,ORP,2.12", ProductId.ORP, FirmwareIssue.I2C_SLEEP_WAKE, True),
        ("?i,ORP,2.13", ProductId.ORP, FirmwareIssue.I2C_SLEEP_WAKE, False),
        ("?i,EC,2.11", ProductId.EC, FirmwareIssue.I2C_SLEEP_WAKE, True),
        ("?i,EC,2.12", ProductId.EC, FirmwareIssue.I2C_SLEEP_WAKE, False),
        ("?i,RTD,2.10", ProductId.RTD, FirmwareIssue.FIRST_READING_INVALID, True),
        ("?i,RTD,2.11", ProductId.RTD, FirmwareIssue.FIRST_READING_INVALID, False),
        ("?i,DO,1.00", ProductId.DO, FirmwareIssue.I2C_SLEEP_WAKE, False),
    ],
)
def test_issue_lookup_from_a_device_reply(reply, product_id, issue, expected):
    device = FakeI2CDevice(address=99)
    device.set_response(i2c_response(DeviceStatus.SUCCESS, reply))
    info = control.read_info_i2c(device)
    assert info.product_id is product_id
    assert product.firmware_has_issue(info.product_id, issue, info.firmware) is expected
