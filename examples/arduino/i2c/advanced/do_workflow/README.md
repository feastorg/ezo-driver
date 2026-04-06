# Arduino I2C D.O. Workflow

This sketch inspects D.O. output selection, temperature, salinity, pressure compensation, calibration state, and the current reading.

## Use It

- Set `DEVICE_I2C_ADDRESS` and the planned compensation values near the top of the sketch.
- Leave `APPLY_CHANGES=0` to inspect only, then re-upload with `APPLY_CHANGES=1` if you want to persist changes.
- These defaults are for operational workflow use, not the vendor calibration baseline. Use `../do_calibration/do_calibration.ino` for calibration.
