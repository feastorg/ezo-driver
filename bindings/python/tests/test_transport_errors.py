"""EzoTransportError carries errno, bus and address where the transport knows them."""

from __future__ import annotations

import errno
import os

import pytest

from ezo_driver import EzoError, EzoTransportError, LinuxI2CDevice, control, i2c, ph
from ezo_driver._ffi import lib
from ezo_driver.enums import ProductId

from ._fakes import ffi as fake_ffi, lib as fake_lib
from ._support import FakeI2CDevice, FakeUARTDevice

TRANSPORT_FAILURE = 3
MISSING_BUS = 999

I2C_CALLS = [
    pytest.param(lambda dev: dev.send_read(), id="send_read"),
    pytest.param(lambda dev: dev.send_command("i"), id="send_command"),
    pytest.param(lambda dev: dev.read_response(), id="read_response"),
    pytest.param(ph.send_read_i2c, id="ph.send_read_i2c"),
    pytest.param(ph.read_response_i2c, id="ph.read_response_i2c"),
    pytest.param(lambda dev: control.send_info_query_i2c(dev, ProductId.PH), id="control.send_info_query_i2c"),
    pytest.param(control.read_info_i2c, id="control.read_info_i2c"),
]


@pytest.mark.skipif(os.path.exists(f"/dev/i2c-{MISSING_BUS}"), reason=f"/dev/i2c-{MISSING_BUS} exists")
def test_linux_open_on_a_missing_bus_carries_errno_bus_and_address():
    with pytest.raises(EzoTransportError) as raised:
        LinuxI2CDevice(bus=MISSING_BUS, address=0x63)

    assert type(raised.value) is EzoTransportError
    assert str(raised.value) == "transport failure"
    assert raised.value.errno == errno.ENOENT
    assert raised.value.bus == MISSING_BUS
    assert raised.value.address == 0x63


class _OpenPathInsteadOfBus:
    """Stands in for the native lib in ``ezo_driver.i2c`` during one test.

    ``ezo_linux_i2c_device_open_bus`` opens ``path`` with the real
    ``ezo_linux_i2c_device_open_path`` instead of ``/dev/i2c-N``, so the real
    constructor and the real Linux transport run on a host with no bus.
    """

    def __init__(self, path: bytes):
        self.path = path

    def __getattr__(self, name):
        return getattr(lib, name)

    def ezo_linux_i2c_device_open_bus(self, device, bus, address):
        return lib.ezo_linux_i2c_device_open_path(device, self.path, address)


@pytest.mark.skipif(not os.path.exists("/dev/null"), reason="needs /dev/null")
@pytest.mark.parametrize(
    "call",
    [
        pytest.param(lambda dev: dev.send_read(), id="send_read"),
        pytest.param(lambda dev: dev.read_response(), id="read_response"),
    ],
)
def test_linux_ioctl_failure_carries_the_kernel_errno(call, monkeypatch):
    # ioctl(I2C_SLAVE) on a file that is not an I2C adapter fails with ENOTTY.
    monkeypatch.setattr(i2c, "lib", _OpenPathInsteadOfBus(b"/dev/null"))
    dev = LinuxI2CDevice(bus=4, address=0x61)

    with pytest.raises(EzoTransportError) as raised:
        call(dev)

    assert type(raised.value) is EzoTransportError
    assert str(raised.value) == "transport failure"
    assert raised.value.errno == errno.ENOTTY
    assert raised.value.bus == 4
    assert raised.value.address == 0x61
    dev.close()


class _OpenOntoFakeTransport:
    """Stands in for the native lib in ``ezo_driver.i2c`` during one test.

    ``ezo_linux_i2c_device_open_bus`` wires the device's core to the fake
    transport instead of opening ``/dev/i2c-N``, so the real constructor runs
    on a host with no bus. The fake fails the call, and the test sets the
    ``last_errno`` the Linux transport would have recorded.
    """

    def __init__(self):
        self.fake = fake_ffi.new("ezo_fake_i2c_transport_t *")
        fake_lib.ezo_fake_i2c_transport_init(self.fake)

    def __getattr__(self, name):
        return getattr(lib, name)

    def ezo_linux_i2c_device_open_bus(self, device, bus, address):
        return lib.ezo_device_init(
            lib.ezo_linux_i2c_device_core(device), address, fake_lib.ezo_fake_i2c_transport_vtable(), self.fake
        )


@pytest.mark.parametrize("call", I2C_CALLS)
def test_linux_transport_failure_after_open_carries_errno_bus_and_address(call, monkeypatch):
    native = _OpenOntoFakeTransport()
    monkeypatch.setattr(i2c, "lib", native)
    dev = LinuxI2CDevice(bus=1, address=0x64)
    native.fake.callback_result = TRANSPORT_FAILURE
    dev._cdev._device.context.last_errno = errno.EREMOTEIO

    with pytest.raises(EzoTransportError) as raised:
        call(dev)

    assert str(raised.value) == "transport failure"
    assert raised.value.errno == errno.EREMOTEIO
    assert raised.value.bus == 1
    assert raised.value.address == 0x64


@pytest.mark.parametrize("call", I2C_CALLS)
def test_fake_i2c_transport_failure_has_no_errno_or_bus(call):
    dev = FakeI2CDevice(address=0x65)
    dev.callback_result = TRANSPORT_FAILURE

    with pytest.raises(EzoTransportError) as raised:
        call(dev)

    assert type(raised.value) is EzoTransportError
    assert str(raised.value) == "transport failure"
    assert raised.value.errno is None
    assert raised.value.bus is None
    assert raised.value.address == 0x65


def test_uart_transport_failure_has_no_errno_bus_or_address():
    dev = FakeUARTDevice()
    dev.write_result = TRANSPORT_FAILURE

    with pytest.raises(EzoTransportError) as raised:
        ph.send_read_uart(dev)

    assert type(raised.value) is EzoTransportError
    assert str(raised.value) == "transport failure"
    assert raised.value.errno is None
    assert raised.value.bus is None
    assert raised.value.address is None


def test_except_ezo_transport_error_still_catches_it():
    dev = FakeI2CDevice()
    dev.callback_result = TRANSPORT_FAILURE

    try:
        dev.send_read()
    except EzoTransportError as caught:
        assert isinstance(caught, EzoError)
    else:
        pytest.fail("no EzoTransportError raised")


def test_constructed_without_attributes_they_default_to_none():
    error = EzoTransportError("bus gone")

    assert str(error) == "bus gone"
    assert error.args == ("bus gone",)
    assert (error.errno, error.bus, error.address) == (None, None, None)
