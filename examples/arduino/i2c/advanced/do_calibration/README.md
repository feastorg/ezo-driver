# Arduino I2C D.O. Calibration

This sketch stages dissolved-oxygen calibration with explicit compensation-state inspection and post-apply verification.

## Use It

- Set `DEVICE_I2C_ADDRESS` and `CALIBRATION_STEP`.
- Leave `APPLY_CHANGES=0` first and verify the compensation state is at the vendor calibration defaults: `20.0 C`, `0.0 ppt`, and `101.0 kPa`.
- Calibrate the low point first with `STEP_ZERO`, then the atmospheric high point with `STEP_HIGH`.
- Use `STEP_STATUS` afterward to confirm the new calibration level.
