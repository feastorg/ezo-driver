# Arduino I2C RTD Calibration

This sketch stages RTD single-point offset calibration against a known reference temperature.

## Use It

- Set `DEVICE_I2C_ADDRESS`, `REFERENCE_TEMPERATURE_C`, and `CALIBRATION_STEP`.
- Leave `APPLY_CHANGES=0` first and verify the probe is stable at the known reference temperature.
- Re-upload with `APPLY_CHANGES=1` to calibrate or use `STEP_CLEAR` to clear calibration.
