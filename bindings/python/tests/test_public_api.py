from __future__ import annotations

import importlib
import importlib.metadata
import re

import pytest

import ezo_driver as ezo
from ezo_driver import i2c, uart

from ._fakes import source_dir


def test_top_level_surface_is_curated():
    assert ezo.LinuxI2CDevice is not None
    assert ezo.LinuxUARTDevice is not None
    assert ezo.EzoError is not None
    assert ezo.CommandKind.GENERIC.value == 0
    assert ezo.DeviceStatus.SUCCESS.value == 1
    assert not hasattr(ezo, "FakeI2CDevice")


def test_version_matches_pyproject():
    pyproject = (source_dir() / "pyproject.toml").read_text()
    expected = re.search(r'(?m)^version\s*=\s*"([^"]+)"', pyproject)
    assert expected is not None
    assert ezo.__version__ == expected.group(1)


def test_version_without_distribution_metadata(monkeypatch):
    def missing(name: str) -> str:
        raise importlib.metadata.PackageNotFoundError(name)

    monkeypatch.setattr(importlib.metadata, "version", missing)
    try:
        assert importlib.reload(ezo).__version__ == "0+unknown"
    finally:
        monkeypatch.undo()
        importlib.reload(ezo)


def test_transport_module_constants_are_exported():
    assert i2c.MAX_RESPONSE_PAYLOAD_LEN > 0
    assert i2c.MAX_TEXT_RESPONSE_LEN > 0
    assert uart.MAX_TEXT_RESPONSE_LEN > 0
    assert uart.MAX_TEXT_RESPONSE_CAPACITY > 0


def test_invalid_constructor_arguments_raise_before_io():
    with pytest.raises(ValueError):
        ezo.LinuxI2CDevice(bus=-1, address=0x63)

    with pytest.raises(ValueError):
        ezo.LinuxI2CDevice(bus=1, address=0x80)

    with pytest.raises(ValueError):
        ezo.LinuxUARTDevice(path="/dev/ttyS0", baud=12345, read_timeout_ms=100)

    with pytest.raises(ValueError):
        ezo.LinuxUARTDevice(path="/dev/ttyS0", baud=9600, read_timeout_ms=0)
