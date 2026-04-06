# Arduino I2C Transport Switch

This sketch inspects I2C transport state and can switch one device from I2C mode to UART mode.

## Use It

- Set `DEVICE_I2C_ADDRESS` and `TARGET_UART_BAUD`.
- Leave `APPLY_CHANGES=0` first and confirm protocol lock is disabled.
- Re-upload with `APPLY_CHANGES=1` to switch the device, then reconnect over UART and use the UART commissioning example.
