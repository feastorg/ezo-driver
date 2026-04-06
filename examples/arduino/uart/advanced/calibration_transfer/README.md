# Arduino UART Calibration Transfer

This sketch exports calibration chunks from one UART-connected device and can optionally apply a trusted import payload to another device of the same product family.

## Use It

- AVR boards use `SoftwareSerial` on pins `10/11`; ESP32 uses `Serial1` on pins `16/17`; boards with `Serial1` use that port directly.
- Set `PRODUCT_ID` for the source or target device.
- Leave `APPLY_IMPORT=0` to export only and capture the printed chunks.
- Only set `IMPORT_PAYLOAD` and `APPLY_IMPORT=1` when you intentionally want to import a trusted payload into a matching device.
