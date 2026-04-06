# Linux I2C Commissioning

These examples are the first stop when the device is already in I2C mode and you want to confirm identity and setup state before moving to typed reads or calibration.

## Examples

- `inspect_device.c`: print device info, repo metadata, and shared control state
- `readiness_check.c`: print product-specific readiness signals such as calibration, outputs, slope, or logger state

## Use It

1. Build with `cmake --preset host-linux-debug` and `cmake --build --preset host-linux-debug --parallel`.
2. Run `./build/host-linux-debug/ezo_linux_i2c_inspect_device_example [device_path] [address]`.
3. Run `./build/host-linux-debug/ezo_linux_i2c_readiness_check_example [device_path] [address]`.
4. Move to `../typed/` when identity looks right, or `../advanced/` when you need calibration or configuration work.

## Notes

- Defaults are `/dev/i2c-1` and the product default I2C address when you omit arguments.
- `readiness_check.c` is the right place to confirm calibration state before you touch a probe in the lab.
