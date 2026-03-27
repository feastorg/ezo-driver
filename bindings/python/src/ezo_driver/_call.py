from __future__ import annotations

from ._ffi import ffi, lib
from .enums import (
    DeviceStatus,
    DOOutputMask,
    DOSalinityUnit,
    ECOutputMask,
    HUMOutputMask,
    MeasurementField,
    ORPExtendedScale,
    PHCalibrationLevel,
    PHExtendedRange,
    ProductCapability,
    ProductCommandFamily,
    ProductDefaultState,
    ProductId,
    ProductOutputSchema,
    ProductSupport,
    ProductTransport,
    RTDScale,
)
from .errors import raise_for_result
from .types import (
    CalibrationExportInfo,
    CalibrationImportResult,
    ControlBaudStatus,
    ControlLedStatus,
    ControlName,
    ControlProtocolLockStatus,
    ControlResponseCodeStatus,
    ControlStatus,
    DeviceInfo,
    DoCalibrationStatus,
    DoOutputConfig,
    DoPressureCompensation,
    DoReading,
    DoSalinityCompensation,
    DoTemperatureCompensation,
    EcCalibrationStatus,
    EcOutputConfig,
    EcProbeK,
    EcReading,
    EcTdsFactor,
    EcTemperatureCompensation,
    HumOutputConfig,
    HumReading,
    HumTemperatureCalibrationStatus,
    MultiOutputReading,
    OrpCalibrationStatus,
    OrpExtendedScaleStatus,
    OrpReading,
    OutputSchema,
    OutputValue,
    PhCalibrationStatus,
    PhExtendedRangeStatus,
    PhReading,
    PhSlope,
    PhTemperatureCompensation,
    ProductMetadata,
    ProductTimingProfile,
    RtdCalibrationStatus,
    RtdLoggerStatus,
    RtdMemoryEntry,
    RtdMemoryStatus,
    RtdMemoryValue,
    RtdReading,
    RtdScaleStatus,
    ScalarReading,
)


def ascii_bytes(value: str | bytes) -> bytes:
    if isinstance(value, bytes):
        return value
    if isinstance(value, str):
        return value.encode("ascii")
    raise TypeError("expected str or bytes")


def ascii_text(value: str | bytes) -> str:
    if isinstance(value, str):
        return value
    if isinstance(value, bytes):
        return value.decode("ascii")
    raise TypeError("expected str or bytes")


def decode_cstr(buffer) -> str:
    return ffi.string(buffer).decode("ascii")


def char_text(value) -> str:
    if isinstance(value, bytes):
        return value.decode("ascii")
    return chr(int(value))


def bool_flag(value: int) -> bool:
    return bool(int(value))


def call_with_timing(func, *args) -> int:
    timing = ffi.new("ezo_timing_hint_t *")
    raise_for_result(int(func(*args, timing)))
    return int(timing.wait_ms)


def call_result(func, *args) -> None:
    raise_for_result(int(func(*args)))


def build_text(func, capacity: int, *args) -> str:
    buffer = ffi.new(f"char[{capacity}]")
    raise_for_result(int(func(buffer, capacity, *args)))
    return decode_cstr(buffer)


def parse_text(func, text: str, ctype: str, converter, *args):
    payload = ascii_bytes(text)
    out = ffi.new(f"{ctype} *")
    raise_for_result(int(func(payload, len(payload), *args, out)))
    return converter(out[0])


def read_struct(func, ctype: str, converter, *args):
    out = ffi.new(f"{ctype} *")
    raise_for_result(int(func(*args, out)))
    return converter(out[0])


def read_struct_with_count(func, ctype: str, count_ctype: str, converter, *args):
    out = ffi.new(f"{ctype} *")
    count = ffi.new(f"{count_ctype} *")
    raise_for_result(int(func(*args, out, count)))
    return converter(out, count[0])


def timing_profile_from_c(raw) -> ProductTimingProfile:
    return ProductTimingProfile(
        generic_ms=int(raw.generic_ms),
        read_ms=int(raw.read_ms),
        read_with_temp_comp_ms=int(raw.read_with_temp_comp_ms),
        calibration_ms=int(raw.calibration_ms),
    )


def device_info_from_c(raw) -> DeviceInfo:
    return DeviceInfo(
        product_id=ProductId(int(raw.product_id)),
        product_code=decode_cstr(raw.product_code),
        firmware_version=decode_cstr(raw.firmware_version),
    )


def product_metadata_from_c(raw_ptr) -> ProductMetadata | None:
    if raw_ptr == ffi.NULL:
        return None
    raw = raw_ptr[0]
    return ProductMetadata(
        product_id=ProductId(int(raw.product_id)),
        family_name=ffi.string(raw.family_name).decode("ascii"),
        vendor_short_code=ffi.string(raw.vendor_short_code).decode("ascii"),
        support_tier=ProductSupport(int(raw.support_tier)),
        default_transport=ProductTransport(int(raw.default_transport)),
        default_i2c_address=int(raw.default_i2c_address),
        default_continuous_mode=ProductDefaultState(int(raw.default_continuous_mode)),
        default_response_codes=ProductDefaultState(int(raw.default_response_codes)),
        default_output_schema=ProductOutputSchema(int(raw.default_output_schema)),
        default_output_count=int(raw.default_output_count),
        capability_flags=ProductCapability(int(raw.capability_flags)),
        command_family_flags=ProductCommandFamily(int(raw.command_family_flags)),
        uart_timing=timing_profile_from_c(raw.uart_timing),
        i2c_timing=timing_profile_from_c(raw.i2c_timing),
    )


def output_value_from_c(raw) -> OutputValue:
    return OutputValue(
        field=MeasurementField(int(raw.field)),
        value=float(raw.value),
        present=bool_flag(raw.present),
    )


def scalar_reading_from_c(raw) -> ScalarReading:
    return ScalarReading(
        field=MeasurementField(int(raw.field)),
        value=float(raw.value),
        present=bool_flag(raw.present),
    )


def output_schema_from_c(raw) -> OutputSchema:
    return OutputSchema(
        product_id=ProductId(int(raw.product_id)),
        fields=tuple(MeasurementField(int(raw.fields[i])) for i in range(int(raw.field_count))),
    )


def multi_output_reading_from_c(raw) -> MultiOutputReading:
    return MultiOutputReading(
        product_id=ProductId(int(raw.product_id)),
        present_mask=int(raw.present_mask),
        values=tuple(output_value_from_c(raw.values[i]) for i in range(int(raw.field_count))),
    )


def control_name_from_c(raw) -> ControlName:
    return ControlName(name=decode_cstr(raw.name))


def control_status_from_c(raw) -> ControlStatus:
    return ControlStatus(restart_code=char_text(raw.restart_code), supply_voltage=float(raw.supply_voltage))


def control_led_status_from_c(raw) -> ControlLedStatus:
    return ControlLedStatus(enabled=bool_flag(raw.enabled))


def control_protocol_lock_status_from_c(raw) -> ControlProtocolLockStatus:
    return ControlProtocolLockStatus(enabled=bool_flag(raw.enabled))


def control_baud_status_from_c(raw) -> ControlBaudStatus:
    return ControlBaudStatus(baud_rate=int(raw.baud_rate))


def control_response_code_status_from_c(raw) -> ControlResponseCodeStatus:
    return ControlResponseCodeStatus(enabled=bool_flag(raw.enabled))


def calibration_export_info_from_c(raw) -> CalibrationExportInfo:
    return CalibrationExportInfo(chunk_count=int(raw.chunk_count), byte_count=int(raw.byte_count))


def calibration_import_result_from_c(raw) -> CalibrationImportResult:
    return CalibrationImportResult(
        device_status=DeviceStatus(int(raw.device_status)),
        pending_reboot=bool_flag(raw.pending_reboot),
    )


def ph_reading_from_c(raw) -> PhReading:
    return PhReading(ph=float(raw.ph))


def ph_temperature_compensation_from_c(raw) -> PhTemperatureCompensation:
    return PhTemperatureCompensation(temperature_c=float(raw.temperature_c))


def ph_calibration_status_from_c(raw) -> PhCalibrationStatus:
    return PhCalibrationStatus(level=PHCalibrationLevel(int(raw.level)))


def ph_slope_from_c(raw) -> PhSlope:
    return PhSlope(
        acid_percent=float(raw.acid_percent),
        base_percent=float(raw.base_percent),
        neutral_mv=float(raw.neutral_mv),
    )


def ph_extended_range_status_from_c(raw) -> PhExtendedRangeStatus:
    return PhExtendedRangeStatus(enabled=PHExtendedRange(int(raw.enabled)))


def orp_reading_from_c(raw) -> OrpReading:
    return OrpReading(millivolts=float(raw.millivolts))


def orp_calibration_status_from_c(raw) -> OrpCalibrationStatus:
    return OrpCalibrationStatus(calibrated=bool_flag(raw.calibrated))


def orp_extended_scale_status_from_c(raw) -> OrpExtendedScaleStatus:
    return OrpExtendedScaleStatus(enabled=ORPExtendedScale(int(raw.enabled)))


def rtd_reading_from_c(raw) -> RtdReading:
    return RtdReading(temperature=float(raw.temperature), scale=RTDScale(int(raw.scale)))


def rtd_scale_status_from_c(raw) -> RtdScaleStatus:
    return RtdScaleStatus(scale=RTDScale(int(raw.scale)))


def rtd_calibration_status_from_c(raw) -> RtdCalibrationStatus:
    return RtdCalibrationStatus(calibrated=bool_flag(raw.calibrated))


def rtd_logger_status_from_c(raw) -> RtdLoggerStatus:
    return RtdLoggerStatus(interval_units=int(raw.interval_units))


def rtd_memory_status_from_c(raw) -> RtdMemoryStatus:
    return RtdMemoryStatus(last_index=int(raw.last_index))


def rtd_memory_entry_from_c(raw) -> RtdMemoryEntry:
    return RtdMemoryEntry(
        index=int(raw.index),
        temperature=float(raw.temperature),
        scale=RTDScale(int(raw.scale)),
    )


def rtd_memory_values_from_c(raw_values, count: int) -> list[RtdMemoryValue]:
    return [
        RtdMemoryValue(
            temperature=float(raw_values[i].temperature),
            scale=RTDScale(int(raw_values[i].scale)),
        )
        for i in range(int(count))
    ]


def ec_reading_from_c(raw) -> EcReading:
    return EcReading(
        present_mask=ECOutputMask(int(raw.present_mask)),
        conductivity_us_cm=float(raw.conductivity_us_cm),
        total_dissolved_solids_ppm=float(raw.total_dissolved_solids_ppm),
        salinity_ppt=float(raw.salinity_ppt),
        specific_gravity=float(raw.specific_gravity),
    )


def ec_output_config_from_c(raw) -> EcOutputConfig:
    return EcOutputConfig(enabled_mask=ECOutputMask(int(raw.enabled_mask)))


def ec_temperature_compensation_from_c(raw) -> EcTemperatureCompensation:
    return EcTemperatureCompensation(temperature_c=float(raw.temperature_c))


def ec_probe_k_from_c(raw) -> EcProbeK:
    return EcProbeK(k_value=float(raw.k_value))


def ec_tds_factor_from_c(raw) -> EcTdsFactor:
    return EcTdsFactor(factor=float(raw.factor))


def ec_calibration_status_from_c(raw) -> EcCalibrationStatus:
    return EcCalibrationStatus(level=int(raw.level))


def do_reading_from_c(raw) -> DoReading:
    return DoReading(
        present_mask=DOOutputMask(int(raw.present_mask)),
        milligrams_per_liter=float(raw.milligrams_per_liter),
        percent_saturation=float(raw.percent_saturation),
    )


def do_output_config_from_c(raw) -> DoOutputConfig:
    return DoOutputConfig(enabled_mask=DOOutputMask(int(raw.enabled_mask)))


def do_temperature_compensation_from_c(raw) -> DoTemperatureCompensation:
    return DoTemperatureCompensation(temperature_c=float(raw.temperature_c))


def do_salinity_compensation_from_c(raw) -> DoSalinityCompensation:
    return DoSalinityCompensation(value=float(raw.value), unit=DOSalinityUnit(int(raw.unit)))


def do_pressure_compensation_from_c(raw) -> DoPressureCompensation:
    return DoPressureCompensation(pressure_kpa=float(raw.pressure_kpa))


def do_calibration_status_from_c(raw) -> DoCalibrationStatus:
    return DoCalibrationStatus(level=int(raw.level))


def hum_reading_from_c(raw) -> HumReading:
    return HumReading(
        present_mask=HUMOutputMask(int(raw.present_mask)),
        relative_humidity_percent=float(raw.relative_humidity_percent),
        air_temperature_c=float(raw.air_temperature_c),
        dew_point_c=float(raw.dew_point_c),
    )


def hum_output_config_from_c(raw) -> HumOutputConfig:
    return HumOutputConfig(enabled_mask=HUMOutputMask(int(raw.enabled_mask)))


def hum_temperature_calibration_status_from_c(raw) -> HumTemperatureCalibrationStatus:
    return HumTemperatureCalibrationStatus(calibrated=bool_flag(raw.calibrated))


__all__ = [
    "ascii_bytes",
    "ascii_text",
    "build_text",
    "call_result",
    "call_with_timing",
    "decode_cstr",
    "ffi",
    "lib",
    "parse_text",
    "read_struct",
    "read_struct_with_count",
    "timing_profile_from_c",
    "device_info_from_c",
    "product_metadata_from_c",
    "output_value_from_c",
    "scalar_reading_from_c",
    "output_schema_from_c",
    "multi_output_reading_from_c",
    "control_name_from_c",
    "control_status_from_c",
    "control_led_status_from_c",
    "control_protocol_lock_status_from_c",
    "control_baud_status_from_c",
    "control_response_code_status_from_c",
    "calibration_export_info_from_c",
    "calibration_import_result_from_c",
    "ph_reading_from_c",
    "ph_temperature_compensation_from_c",
    "ph_calibration_status_from_c",
    "ph_slope_from_c",
    "ph_extended_range_status_from_c",
    "orp_reading_from_c",
    "orp_calibration_status_from_c",
    "orp_extended_scale_status_from_c",
    "rtd_reading_from_c",
    "rtd_scale_status_from_c",
    "rtd_calibration_status_from_c",
    "rtd_logger_status_from_c",
    "rtd_memory_status_from_c",
    "rtd_memory_entry_from_c",
    "rtd_memory_values_from_c",
    "ec_reading_from_c",
    "ec_output_config_from_c",
    "ec_temperature_compensation_from_c",
    "ec_probe_k_from_c",
    "ec_tds_factor_from_c",
    "ec_calibration_status_from_c",
    "do_reading_from_c",
    "do_output_config_from_c",
    "do_temperature_compensation_from_c",
    "do_salinity_compensation_from_c",
    "do_pressure_compensation_from_c",
    "do_calibration_status_from_c",
    "hum_reading_from_c",
    "hum_output_config_from_c",
    "hum_temperature_calibration_status_from_c",
]
