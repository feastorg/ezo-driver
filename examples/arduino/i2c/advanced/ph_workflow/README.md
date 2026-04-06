# Arduino I2C pH Workflow

This sketch inspects pH temperature compensation, calibration state, slope, extended range, and the current reading.

## Use It

- Set `DEVICE_I2C_ADDRESS` and any planned values such as `PLANNED_TEMPERATURE_C`.
- Leave `APPLY_CHANGES=0` to inspect only, then re-upload with `APPLY_CHANGES=1` if you want to persist changes.
- Use this for operational pH state. Use `../ph_calibration/ph_calibration.ino` for buffer calibration.
