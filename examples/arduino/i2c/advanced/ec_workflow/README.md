# Arduino I2C EC Workflow

This sketch inspects EC output selection, temperature compensation, probe K, TDS factor, calibration state, and the current reading.

## Use It

- Set `DEVICE_I2C_ADDRESS` and any planned values such as `PLANNED_TEMPERATURE_C`, `PLANNED_PROBE_K`, and `PLANNED_TDS_FACTOR`.
- Leave `APPLY_CHANGES=0` first, then re-upload with `APPLY_CHANGES=1` if you want to persist those settings.
- Use this for operational EC state. Use `../ec_calibration/ec_calibration.ino` for dry and solution calibration steps.
