# Arduino I2C RTD Workflow

This sketch inspects RTD scale, calibration, logger, memory state, and the current reading, with optional scale/logger/memory changes.

## Use It

- Set `DEVICE_I2C_ADDRESS`, `PLANNED_SCALE`, `PLANNED_LOGGER_INTERVAL_UNITS`, and `CLEAR_MEMORY` as needed.
- Leave `APPLY_CHANGES=0` first, then re-upload with `APPLY_CHANGES=1` if you want to persist scale or logger changes.
- Only clear or bulk-recall memory when the logger is disabled.
