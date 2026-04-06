# Linux I2C Raw

This directory contains the smallest Linux I2C transport example.

## Example

- `raw_command.c`: send one raw command and print the raw text response

## Use It

1. Build with `cmake --preset host-linux-debug` and `cmake --build --preset host-linux-debug --parallel`.
2. Run `./build/host-linux-debug/ezo_linux_i2c_raw_command_example [device_path] [address] [command]`.
3. Start with `/dev/i2c-1`, the device address, and a safe command such as `i`.

## Notes

- This example assumes the device is already in I2C mode.
- Use `../commissioning/` next if you need identity or readiness checks.
