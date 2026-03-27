from __future__ import annotations

from dataclasses import dataclass

from .enums import (
    DOOutputMask,
    DOSalinityUnit,
    DeviceStatus,
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


@dataclass(frozen=True)
class ProductTimingProfile:
    generic_ms: int
    read_ms: int
    read_with_temp_comp_ms: int
    calibration_ms: int


@dataclass(frozen=True)
class DeviceInfo:
    product_id: ProductId
    product_code: str
    firmware_version: str


@dataclass(frozen=True)
class ProductMetadata:
    product_id: ProductId
    family_name: str
    vendor_short_code: str
    support_tier: ProductSupport
    default_transport: ProductTransport
    default_i2c_address: int
    default_continuous_mode: ProductDefaultState
    default_response_codes: ProductDefaultState
    default_output_schema: ProductOutputSchema
    default_output_count: int
    capability_flags: ProductCapability
    command_family_flags: ProductCommandFamily
    uart_timing: ProductTimingProfile
    i2c_timing: ProductTimingProfile


@dataclass(frozen=True)
class OutputValue:
    field: MeasurementField
    value: float
    present: bool


@dataclass(frozen=True)
class ScalarReading:
    field: MeasurementField
    value: float
    present: bool


@dataclass(frozen=True)
class OutputSchema:
    product_id: ProductId
    fields: tuple[MeasurementField, ...]


@dataclass(frozen=True)
class MultiOutputReading:
    product_id: ProductId
    present_mask: int
    values: tuple[OutputValue, ...]


@dataclass(frozen=True)
class ControlName:
    name: str


@dataclass(frozen=True)
class ControlStatus:
    restart_code: str
    supply_voltage: float


@dataclass(frozen=True)
class ControlLedStatus:
    enabled: bool


@dataclass(frozen=True)
class ControlProtocolLockStatus:
    enabled: bool


@dataclass(frozen=True)
class ControlBaudStatus:
    baud_rate: int


@dataclass(frozen=True)
class ControlResponseCodeStatus:
    enabled: bool


@dataclass(frozen=True)
class CalibrationExportInfo:
    chunk_count: int
    byte_count: int


@dataclass(frozen=True)
class CalibrationImportResult:
    device_status: DeviceStatus
    pending_reboot: bool


@dataclass(frozen=True)
class PhReading:
    ph: float


@dataclass(frozen=True)
class PhTemperatureCompensation:
    temperature_c: float


@dataclass(frozen=True)
class PhCalibrationStatus:
    level: PHCalibrationLevel


@dataclass(frozen=True)
class PhSlope:
    acid_percent: float
    base_percent: float
    neutral_mv: float


@dataclass(frozen=True)
class PhExtendedRangeStatus:
    enabled: PHExtendedRange


@dataclass(frozen=True)
class OrpReading:
    millivolts: float


@dataclass(frozen=True)
class OrpCalibrationStatus:
    calibrated: bool


@dataclass(frozen=True)
class OrpExtendedScaleStatus:
    enabled: ORPExtendedScale


@dataclass(frozen=True)
class RtdReading:
    temperature: float
    scale: RTDScale


@dataclass(frozen=True)
class RtdScaleStatus:
    scale: RTDScale


@dataclass(frozen=True)
class RtdCalibrationStatus:
    calibrated: bool


@dataclass(frozen=True)
class RtdLoggerStatus:
    interval_units: int


@dataclass(frozen=True)
class RtdMemoryStatus:
    last_index: int


@dataclass(frozen=True)
class RtdMemoryEntry:
    index: int
    temperature: float
    scale: RTDScale


@dataclass(frozen=True)
class RtdMemoryValue:
    temperature: float
    scale: RTDScale


@dataclass(frozen=True)
class EcReading:
    present_mask: ECOutputMask
    conductivity_us_cm: float
    total_dissolved_solids_ppm: float
    salinity_ppt: float
    specific_gravity: float


@dataclass(frozen=True)
class EcOutputConfig:
    enabled_mask: ECOutputMask


@dataclass(frozen=True)
class EcTemperatureCompensation:
    temperature_c: float


@dataclass(frozen=True)
class EcProbeK:
    k_value: float


@dataclass(frozen=True)
class EcTdsFactor:
    factor: float


@dataclass(frozen=True)
class EcCalibrationStatus:
    level: int


@dataclass(frozen=True)
class DoReading:
    present_mask: DOOutputMask
    milligrams_per_liter: float
    percent_saturation: float


@dataclass(frozen=True)
class DoOutputConfig:
    enabled_mask: DOOutputMask


@dataclass(frozen=True)
class DoTemperatureCompensation:
    temperature_c: float


@dataclass(frozen=True)
class DoSalinityCompensation:
    value: float
    unit: DOSalinityUnit


@dataclass(frozen=True)
class DoPressureCompensation:
    pressure_kpa: float


@dataclass(frozen=True)
class DoCalibrationStatus:
    level: int


@dataclass(frozen=True)
class HumReading:
    present_mask: HUMOutputMask
    relative_humidity_percent: float
    air_temperature_c: float
    dew_point_c: float


@dataclass(frozen=True)
class HumOutputConfig:
    enabled_mask: HUMOutputMask


@dataclass(frozen=True)
class HumTemperatureCalibrationStatus:
    calibrated: bool
