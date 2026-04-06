# Arduino UART Transport Switch

This sketch inspects UART transport state and can switch one device from UART mode to I2C mode.

## Use It

- AVR boards use `SoftwareSerial` on pins `10/11`; ESP32 uses `Serial1` on pins `16/17`; boards with `Serial1` use that port directly.
- Set `TARGET_I2C_ADDRESS`.
- Leave `APPLY_CHANGES=0` first and confirm protocol lock is disabled.
- Re-upload with `APPLY_CHANGES=1` to switch the device, then reconnect over I2C and use the I2C commissioning example.
