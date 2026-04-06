# Arduino UART Inspect Device

This sketch bootstraps response-code mode when needed and prints identity and status information for one UART-connected circuit.

## Use It

- Wire the sensor UART to the stream selected by the sketch.
- On boards without `Serial1`, the sketch uses `Serial` as the sensor stream, so you do not get USB debug output.
- On boards with a separate debug stream, upload the sketch and open Serial Monitor at `115200`.
