from __future__ import annotations

from importlib.metadata import PackageNotFoundError as _PackageNotFoundError
from importlib.metadata import version as _distribution_version

from .errors import (
    EzoArgumentError,
    EzoDeviceStatusError,
    EzoError,
    EzoNotReadyError,
    EzoParseError,
    EzoProtocolError,
    EzoTransportError,
)
from .enums import (
    CommandKind,
    DeviceStatus,
    FirmwareFeature,
    FirmwareIssue,
    MeasurementField,
    ProductId,
    ProductSupport,
    ProductTransport,
    UARTResponseKind,
)
from .i2c import LinuxI2CDevice
from .types import (
    DeviceInfo,
    FirmwareVersion,
    MultiOutputReading,
    OutputSchema,
    OutputValue,
    ProductMetadata,
    ProductTimingProfile,
    ScalarReading,
)
from .uart import LinuxUARTDevice

# A source tree used without installing has no distribution metadata.
try:
    __version__: str = _distribution_version("ezo-driver")
except _PackageNotFoundError:
    __version__ = "0+unknown"

__all__ = [
    "LinuxI2CDevice",
    "LinuxUARTDevice",
    "EzoError",
    "EzoArgumentError",
    "EzoTransportError",
    "EzoProtocolError",
    "EzoNotReadyError",
    "EzoDeviceStatusError",
    "EzoParseError",
    "CommandKind",
    "DeviceStatus",
    "UARTResponseKind",
    "ProductId",
    "ProductSupport",
    "ProductTransport",
    "MeasurementField",
    "ProductTimingProfile",
    "DeviceInfo",
    "FirmwareVersion",
    "FirmwareFeature",
    "FirmwareIssue",
    "ProductMetadata",
    "OutputValue",
    "ScalarReading",
    "OutputSchema",
    "MultiOutputReading",
]
