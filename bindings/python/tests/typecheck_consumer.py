# pyright: reportMissingTypeStubs=true
"""A consumer of the installed package, checked by pyright in CI and never run.

pyright reports an import without `py.typed` as a missing stub, and each
`assert_type` pins a public return type, so this fails if the wheel loses
`py.typed` or a public helper loses its return annotation.
"""

from __future__ import annotations

from typing_extensions import assert_type

import ezo_driver
from ezo_driver import DeviceStatus, LinuxI2CDevice, LinuxUARTDevice, ProductId, UARTResponseKind
from ezo_driver import calibration_transfer, control, do, ec, hum, orp, ph, product, rtd, schema
from ezo_driver.types import (
    CalibrationExportInfo,
    DeviceInfo,
    DoReading,
    EcReading,
    HumReading,
    MultiOutputReading,
    OrpReading,
    OutputSchema,
    PhReading,
    ProductMetadata,
    RtdMemoryValue,
    RtdReading,
)


def check_version() -> None:
    assert_type(ezo_driver.__version__, str)


def check_status_error(error: ezo_driver.EzoDeviceStatusError) -> None:
    assert_type(error.status, DeviceStatus)


def check_i2c(dev: LinuxI2CDevice) -> None:
    with dev as opened:
        assert_type(opened, LinuxI2CDevice)
        assert_type(control.send_info_query_i2c(opened, ProductId.PH), int)
        assert_type(control.read_info_i2c(opened), DeviceInfo)
        assert_type(ph.send_read_i2c(opened), int)
        assert_type(ph.read_response_i2c(opened), PhReading)
        assert_type(orp.read_response_i2c(opened), OrpReading)
        assert_type(ec.read_response_i2c(opened, 0x1), EcReading)
        assert_type(do.read_response_i2c(opened, 0x1), DoReading)
        assert_type(hum.read_response_i2c(opened, 0x1), HumReading)
        assert_type(rtd.read_response_i2c(opened, 0), RtdReading)
        assert_type(rtd.read_memory_all_i2c(opened, 0), list[RtdMemoryValue])
        assert_type(calibration_transfer.read_export_info_i2c(opened), CalibrationExportInfo)
        assert_type(opened.read_response(), tuple[DeviceStatus, str])


def check_uart(dev: LinuxUARTDevice) -> None:
    with dev as opened:
        assert_type(opened, LinuxUARTDevice)
        assert_type(ph.send_read_uart(opened), int)
        assert_type(ph.read_response_uart(opened), PhReading)
        assert_type(opened.read_line(), tuple[UARTResponseKind, str])


def check_metadata() -> None:
    assert_type(product.product_id_from_short_code("pH"), ProductId)
    assert_type(product.get_metadata(ProductId.PH), ProductMetadata | None)
    out_schema = schema.get_output_schema(ProductId.EC)
    assert_type(out_schema, OutputSchema)
    assert_type(schema.parse_multi_output_reading("1,2", out_schema, 0x3), MultiOutputReading)
