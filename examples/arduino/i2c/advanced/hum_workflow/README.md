# Arduino I2C HUM Workflow

This sketch inspects HUM output selection, temperature-calibration state, and the current reading, with optional output and reference-temperature updates.

## Use It

- Set `DEVICE_I2C_ADDRESS`, `ENABLE_DEW_POINT_OUTPUT`, and `PLANNED_REFERENCE_TEMPERATURE_C`.
- Leave `APPLY_CHANGES=0` to inspect only, then re-upload with `APPLY_CHANGES=1` if you want to persist changes.
- Use `../hum_temperature_calibration/hum_temperature_calibration.ino` for the staged temperature-calibration flow.
