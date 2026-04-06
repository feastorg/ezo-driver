# Arduino I2C Control Workflow

This sketch inspects shared control state such as name, LED, protocol lock, status, and optional admin operations like find, sleep, and factory reset.

## Use It

- Set `DEVICE_I2C_ADDRESS` and any planned values such as `PLANNED_NAME`, `PLANNED_LED`, or `PLANNED_PROTOCOL_LOCK`.
- Leave `APPLY_CHANGES=0` first and inspect the current state.
- Re-upload with `APPLY_CHANGES=1` only when you actually want to apply the selected control action.
