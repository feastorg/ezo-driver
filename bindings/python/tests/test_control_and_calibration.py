from __future__ import annotations

from ezo_driver import DeviceStatus, ProductId, UARTResponseKind, calibration_transfer, control

from ._support import FakeI2CDevice, FakeUARTDevice, i2c_response, uart_lines


def test_control_helpers_cover_i2c_and_uart_sequences():
    i2c_dev = FakeI2CDevice(address=99)

    wait_ms = control.send_name_set_i2c(i2c_dev, ProductId.PH, "tank")
    assert wait_ms == 300
    assert i2c_dev.last_tx == b"Name,tank"

    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "?i,pH,2.16"))
    wait_ms = control.send_info_query_i2c(i2c_dev, ProductId.PH)
    assert wait_ms == 300
    info = control.read_info_i2c(i2c_dev)
    assert info.product_id is ProductId.PH
    assert info.firmware_version == "2.16"

    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "?STATUS,P,4.89"))
    wait_ms = control.send_status_query_i2c(i2c_dev, ProductId.PH)
    assert wait_ms == 300
    status = control.read_status_i2c(i2c_dev)
    assert status.restart_code == "P"

    uart_dev = FakeUARTDevice()

    wait_ms = control.send_led_set_uart(uart_dev, ProductId.HUM, True)
    assert wait_ms == 300
    assert uart_dev.tx_bytes == b"L,1\r"

    uart_dev.set_response(uart_lines("?Name,mix", "*OK"))
    wait_ms = control.send_name_query_uart(uart_dev, ProductId.HUM)
    assert wait_ms == 300
    name = control.read_name_uart(uart_dev)
    assert name.name == "mix"

    uart_dev.set_response(uart_lines("?Baud,9600", "*OK"))
    wait_ms = control.send_baud_query_uart(uart_dev, ProductId.HUM)
    assert wait_ms == 300
    baud = control.read_baud_uart(uart_dev)
    assert baud.baud_rate == 9600

    uart_dev.set_response(uart_lines("?*OK,0"))
    wait_ms = control.send_response_code_query_uart(uart_dev, ProductId.HUM)
    assert wait_ms == 300
    response_code = control.read_response_code_uart(uart_dev)
    assert response_code.enabled is False


def test_calibration_transfer_helpers_cover_export_and_import_flows():
    assert calibration_transfer.parse_export_info("10,120").chunk_count == 10
    assert calibration_transfer.build_import_command(b"AAAABBBB") == "Import,AAAABBBB"

    i2c_dev = FakeI2CDevice(address=99)
    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "10,120"))
    wait_ms = calibration_transfer.send_export_info_query_i2c(i2c_dev, ProductId.PH)
    assert wait_ms == 300
    info = calibration_transfer.read_export_info_i2c(i2c_dev)
    assert info.byte_count == 120

    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, b"AAAA"))
    wait_ms = calibration_transfer.send_export_next_i2c(i2c_dev, ProductId.PH)
    assert wait_ms == 300
    assert calibration_transfer.read_export_chunk_i2c(i2c_dev) == b"AAAA"

    wait_ms = calibration_transfer.send_import_i2c(i2c_dev, ProductId.PH, b"BBBB")
    assert wait_ms == 300
    assert i2c_dev.last_tx == b"Import,BBBB"

    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, b""))
    assert calibration_transfer.read_import_status_i2c(i2c_dev) is DeviceStatus.SUCCESS

    i2c_dev.set_response(i2c_response(DeviceStatus.SUCCESS, "*Pending"))
    import_result = calibration_transfer.read_import_result_i2c(i2c_dev)
    assert import_result.device_status is DeviceStatus.SUCCESS
    assert import_result.pending_reboot is True

    uart_dev = FakeUARTDevice()
    uart_dev.set_response(uart_lines("10,120", "*OK"))
    wait_ms = calibration_transfer.send_export_info_query_uart(uart_dev, ProductId.PH)
    assert wait_ms == 300
    info = calibration_transfer.read_export_info_uart(uart_dev)
    assert info.chunk_count == 10

    uart_dev.set_response(uart_lines("AAAA", "*DONE"))
    wait_ms = calibration_transfer.send_export_next_uart(uart_dev, ProductId.PH)
    assert wait_ms == 300
    kind, payload = calibration_transfer.read_export_chunk_uart(uart_dev)
    assert kind is UARTResponseKind.DATA
    assert payload == b"AAAA"
    kind, payload = calibration_transfer.read_export_chunk_uart(uart_dev)
    assert kind is UARTResponseKind.DONE
    assert payload == b"*DONE"

    wait_ms = calibration_transfer.send_import_uart(uart_dev, ProductId.PH, "BBBB")
    assert wait_ms == 300
    assert uart_dev.tx_bytes.endswith(b"Import,BBBB\r")

    uart_dev.set_response(uart_lines("*OK"))
    assert calibration_transfer.read_import_result_uart(uart_dev) is UARTResponseKind.OK
