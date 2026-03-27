from __future__ import annotations

import pytest

from ezo_driver import (
    CommandKind,
    DeviceStatus,
    MeasurementField,
    ProductId,
    ProductSupport,
    ProductTransport,
    UARTResponseKind,
    base,
    ec,
    enums,
    i2c,
    parse,
    product,
    schema,
    uart,
)
from ezo_driver.errors import EzoError
from ezo_driver.enums import ECOutputMask, ProductCapability

from ._support import FakeI2CDevice, FakeUARTDevice, i2c_response, uart_lines


def test_i2c_raw_transport_helpers_cover_text_and_raw_reads():
    dev = FakeI2CDevice(address=99)

    wait_ms = dev.send_command("name,?", CommandKind.GENERIC)
    assert wait_ms == 300
    assert dev.last_tx == b"name,?"

    dev.set_response(i2c_response(DeviceStatus.SUCCESS, "7.12"))
    status, text = dev.read_response()
    assert status is DeviceStatus.SUCCESS
    assert text == "7.12"
    assert dev.last_status is DeviceStatus.SUCCESS
    assert i2c.device_status_name(status) == "EZO_STATUS_SUCCESS"

    dev.set_response(bytes([DeviceStatus.SUCCESS, ord("A"), 0, ord("B"), 0x7F]))
    status, payload = dev.read_response_raw()
    assert status is DeviceStatus.SUCCESS
    assert payload == b"A\x00B\x7f"


def test_uart_raw_transport_helpers_cover_line_parsing_and_terminal_kinds():
    dev = FakeUARTDevice()
    dev.max_bytes_per_read = 2

    wait_ms = dev.send_command("i", CommandKind.GENERIC)
    assert wait_ms == 300
    assert dev.tx_bytes == b"i\r"

    dev.set_response(uart_lines("12.34", "*OK"))
    kind, text = dev.read_line()
    assert kind is UARTResponseKind.DATA
    assert text == "12.34"

    kind, text = dev.read_line()
    assert kind is UARTResponseKind.OK
    assert text == "*OK"
    assert uart.response_kind_is_control(kind)
    assert uart.response_kind_is_terminal(kind)


def test_uart_read_line_raises_on_oversized_payload():
    dev = FakeUARTDevice()
    dev.set_response(uart_lines("7" * uart.MAX_TEXT_RESPONSE_CAPACITY))

    with pytest.raises(EzoError):
        dev.read_line()


def test_base_product_parse_and_schema_helpers_cover_shared_surface():
    assert base.get_timing_hint_for_command_kind(CommandKind.READ) == 1000
    assert base.result_name(enums.ResultCode.OK) == "EZO_OK"
    assert base.parse_double("12.34") == pytest.approx(12.34)
    assert base.format_fixed_command("t,", 1.005, 2) == "t,1.01"

    assert parse.text_span_is_empty("   ")
    assert parse.text_span_equals(" ?Status ", "?Status")
    assert parse.parse_text_span_uint32("225") == 225
    assert parse.parse_text_span_double(" -4.50 ") == pytest.approx(-4.5)
    assert parse.parse_csv_fields(" 12.30 ,   , -4.50 ") == ["12.30", "", "-4.50"]
    assert parse.parse_query_response(" ?Status, P , 5.038 ") == ("?Status", ["P", "5.038"])
    assert parse.parse_prefixed_fields("?T,19.5", "?T") == ["19.5"]

    sequence = parse.UARTSequence()
    assert sequence.push_line(UARTResponseKind.DATA) == enums.UARTSequenceStep.DATA
    assert not sequence.is_complete()
    assert sequence.push_line(UARTResponseKind.OK) == enums.UARTSequenceStep.TERMINAL
    assert sequence.is_complete()
    assert sequence.terminal_kind is UARTResponseKind.OK

    assert product.product_id_from_short_code(" D.O. ") is ProductId.DO
    info = product.parse_device_info("?I,DO,2.16")
    assert info.product_id is ProductId.DO
    assert info.product_code == "DO"

    meta = product.get_metadata(ProductId.HUM)
    assert meta is not None
    assert meta.support_tier is ProductSupport.FULL
    assert meta.default_transport is ProductTransport.UART
    assert product.get_support_tier(ProductId.ORP) is ProductSupport.FULL
    assert product.supports_capability(ProductId.RTD, ProductCapability.DATA_LOGGING)
    assert product.get_timing_hint(ProductId.PH, ProductTransport.UART, CommandKind.READ) == 900
    assert product.resolve_timing_hint(ProductId.UNKNOWN, ProductTransport.I2C, CommandKind.READ) == 1000

    ec_schema = schema.get_output_schema(ProductId.EC)
    assert ec_schema.product_id is ProductId.EC
    assert ec_schema.fields[:2] == (
        MeasurementField.CONDUCTIVITY,
        MeasurementField.TOTAL_DISSOLVED_SOLIDS,
    )
    assert schema.count_enabled_fields(ec_schema, int(ECOutputMask.CONDUCTIVITY | ECOutputMask.SALINITY)) == 2

    scalar = schema.parse_scalar_reading(" -245.7 ", MeasurementField.ORP)
    assert scalar.field is MeasurementField.ORP
    assert scalar.value == pytest.approx(-245.7)

    multi = schema.parse_multi_output_reading(
        "100.00,1.234",
        ec_schema,
        int(ECOutputMask.CONDUCTIVITY | ECOutputMask.SALINITY),
    )
    assert multi.product_id is ProductId.EC
    assert multi.present_mask == int(ECOutputMask.CONDUCTIVITY | ECOutputMask.SALINITY)
    assert multi.values[0].present is True
    assert multi.values[2].present is True


@pytest.mark.parametrize(
    ("builder", "args"),
    [
        (base.format_fixed_command, ("t,", 1.0, 10)),
        (ec.build_temperature_command, (25.0, 10)),
    ],
)
def test_decimal_validation_is_consistent(builder, args):
    with pytest.raises(ValueError):
        builder(*args)
