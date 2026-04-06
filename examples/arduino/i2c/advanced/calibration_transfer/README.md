# Arduino I2C Calibration Transfer

This sketch exports calibration chunks from one device and can optionally apply a trusted import payload to another device of the same product family.

## Use It

- Set `DEVICE_I2C_ADDRESS` and `PRODUCT_ID`.
- Leave `APPLY_IMPORT=0` to export only. Upload the sketch and capture the printed chunk payloads.
- Only set `IMPORT_PAYLOAD` and `APPLY_IMPORT=1` when you intentionally want to import a trusted payload into a matching device.
