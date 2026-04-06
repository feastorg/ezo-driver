# Arduino UART Raw Smoke

This sketch is the smallest Arduino UART transport check: it sends a raw command and prints the response when a separate debug stream exists.

## Use It

- Wire the sensor UART to the stream selected by the sketch.
- On boards without `Serial1`, the sketch uses `Serial` as the sensor stream, so you do not get USB debug output.
- On boards with a separate debug stream, upload the sketch and open Serial Monitor at `115200`.
