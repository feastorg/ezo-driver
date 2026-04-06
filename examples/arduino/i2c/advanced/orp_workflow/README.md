# Arduino I2C ORP Workflow

This sketch inspects ORP reading, calibration state, and extended-scale state, with an optional extended-scale apply path.

## Use It

- Set `DEVICE_I2C_ADDRESS` and `ENABLE_EXTENDED_SCALE` as needed.
- Leave `APPLY_CHANGES=0` for inspection, then re-upload with `APPLY_CHANGES=1` only if you want to change the extended-scale setting.
- Use `../orp_calibration/orp_calibration.ino` for the actual 225 mV calibration flow.
