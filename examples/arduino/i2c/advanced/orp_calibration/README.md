# Arduino I2C ORP Calibration

This sketch stages ORP single-point calibration with bounded preview reads.

## Use It

- Set `DEVICE_I2C_ADDRESS`, `REFERENCE_MV`, and `CALIBRATION_STEP`.
- Leave `APPLY_CHANGES=0` first and confirm the probe is stable in the reference solution.
- Re-upload with `APPLY_CHANGES=1` to apply the `225 mV` point, or use `STEP_CLEAR` to clear calibration.
