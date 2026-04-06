# Arduino UART Multi-Device Router

This sketch drives one shared UART transport across multiple routed EZO channels with explicit route-select pins.

## Use It

- Wire the route-select pins shown at the top of the sketch for your board family and update the routed-module table if your channels differ.
- AVR boards use `SoftwareSerial` on pins `10/11`; ESP32 uses `Serial1` on pins `16/17`; boards with `Serial1` use that port directly.
- Upload the sketch and open Serial Monitor at `115200`.
- This is a hardware-topology-specific example for a routed UART setup, not a generic single-device UART starting point.
