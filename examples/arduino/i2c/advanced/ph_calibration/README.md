# Arduino I2C pH Calibration

This sketch stages pH calibration one step at a time with preview reads and explicit midpoint-first ordering.

## Use It

- Set `DEVICE_I2C_ADDRESS`, `CALIBRATION_STEP`, and `REFERENCE_PH`.
- Leave `APPLY_CHANGES=0` first, upload, and confirm the preview readings are stable in the matching buffer.
- Re-upload with `APPLY_CHANGES=1` to commit the step. Do midpoint (`7.00`) first, then low (`4.00`), then high (`10.00`).
- Use `STEP_STATUS` afterward to confirm calibration level and post-calibration slope output.
