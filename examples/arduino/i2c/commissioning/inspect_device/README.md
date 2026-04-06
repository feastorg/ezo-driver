# Arduino I2C Inspect Device

This sketch prints identity and status information for one I2C circuit so you can confirm product type, firmware, and basic reachability before using typed or calibration examples.

## Use It

- Set `DEVICE_I2C_ADDRESS` for the attached circuit.
- Upload the sketch and open Serial Monitor at `115200`.
- Use it before typed reads if you are not fully sure which EZO board is attached or what address it is using.
