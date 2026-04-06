# Arduino I2C HUM Temperature Calibration

This sketch stages the HUM circuit's temperature calibration only. Humidity calibration remains at the vendor factory default.

## Use It

- Set `DEVICE_I2C_ADDRESS`, `REFERENCE_TEMPERATURE_C`, and `CALIBRATION_STEP`.
- Leave `APPLY_CHANGES=0` first and confirm the reference temperature is stable.
- Re-upload with `APPLY_CHANGES=1` to apply or use `STEP_CLEAR` to clear the temperature calibration.
