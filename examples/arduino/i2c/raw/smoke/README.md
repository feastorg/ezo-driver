# Arduino I2C Raw Smoke

This sketch is the smallest Arduino I2C transport check: it opens the device at `DEVICE_I2C_ADDRESS`, sends a raw command, and prints the response.

## Use It

- Set `DEVICE_I2C_ADDRESS` to the circuit on your bus.
- Upload the sketch and open Serial Monitor at `115200`.
- Use this first when you only want to confirm wiring, address, and basic I2C communication.
