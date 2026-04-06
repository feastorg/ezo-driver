# Arduino I2C EC Calibration

This sketch stages EC calibration and shows the current temperature and probe-K state before you apply changes.

## Use It

- Set `DEVICE_I2C_ADDRESS`, `PROBE_K`, `CALIBRATION_STEP`, and `REFERENCE_US`.
- Keep `APPLY_CHANGES=0` for the first upload and confirm the probe is in the matching dry or solution state.
- Start with the dry step, then use the appropriate single, low, or high reference solution.
- Keep temperature compensation at the vendor default while calibrating.
