"""The I2C status a failed read carries picks the EzoProtocolError subclass."""

from __future__ import annotations

import pytest

from ezo_driver import (
    DeviceStatus,
    EzoDeviceStatusError,
    EzoNotReadyError,
    EzoProtocolError,
    UARTResponseKind,
    calibration_transfer,
    parse,
    ph,
    rtd,
)
from ezo_driver.enums import RTDScale

from ._support import FakeI2CDevice, FakeUARTDevice, i2c_response, uart_lines

# The status byte a circuit sends first (src/ezo_i2c.c, ezo_map_status_byte).
STATUS_BYTES = {DeviceStatus.FAIL: 2, DeviceStatus.NOT_READY: 254, DeviceStatus.NO_DATA: 255}

STATUS_ERRORS = [
    (DeviceStatus.NOT_READY, EzoNotReadyError),
    (DeviceStatus.FAIL, EzoDeviceStatusError),
    (DeviceStatus.NO_DATA, EzoDeviceStatusError),
]

I2C_READS = [
    pytest.param(ph.read_response_i2c, id="ph.read_response_i2c"),
    pytest.param(lambda dev: rtd.read_memory_all_i2c(dev, RTDScale.CELSIUS), id="rtd.read_memory_all_i2c"),
    pytest.param(calibration_transfer.read_export_chunk_i2c, id="calibration_transfer.read_export_chunk_i2c"),
]


@pytest.mark.parametrize("read", I2C_READS)
@pytest.mark.parametrize(("status", "error"), STATUS_ERRORS, ids=lambda value: getattr(value, "name", None))
def test_i2c_status_raises_its_subclass(read, status, error):
    dev = FakeI2CDevice()
    dev.set_response(i2c_response(STATUS_BYTES[status]))

    with pytest.raises(error) as raised:
        read(dev)

    assert type(raised.value) is error
    assert str(raised.value) == "protocol failure"
    assert dev.last_status is status
    if error is EzoDeviceStatusError:
        assert raised.value.status is status


@pytest.mark.parametrize(("status", "error"), STATUS_ERRORS, ids=lambda value: getattr(value, "name", None))
def test_except_ezo_protocol_error_still_catches_each_status(status, error):
    dev = FakeI2CDevice()
    dev.set_response(i2c_response(STATUS_BYTES[status]))

    try:
        ph.read_response_i2c(dev)
    except EzoProtocolError as caught:
        assert isinstance(caught, error)
    else:
        pytest.fail("no EzoProtocolError raised")


def test_i2c_unknown_status_raises_the_base_class():
    dev = FakeI2CDevice()
    dev.set_response(bytes([7]) + b"7.00\0")

    with pytest.raises(EzoProtocolError) as raised:
        ph.read_response_i2c(dev)

    assert type(raised.value) is EzoProtocolError
    assert dev.last_status is DeviceStatus.UNKNOWN


def test_uart_protocol_failure_raises_the_base_class():
    dev = FakeUARTDevice()
    dev.set_response(uart_lines("*ER"))

    with pytest.raises(EzoProtocolError) as raised:
        ph.read_response_uart(dev)

    assert type(raised.value) is EzoProtocolError


def test_parse_only_protocol_failure_raises_the_base_class():
    sequence = parse.UARTSequence()
    sequence.push_line(UARTResponseKind.OK)

    with pytest.raises(EzoProtocolError) as raised:
        sequence.push_line(UARTResponseKind.DATA)

    assert type(raised.value) is EzoProtocolError
