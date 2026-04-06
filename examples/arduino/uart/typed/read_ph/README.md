# Arduino UART Read pH

This sketch performs one typed pH read over Arduino UART and explicitly bootstraps response-code mode first.

## Use It

- Wire the sensor UART to the stream selected by the sketch.
- On boards without `Serial1`, the sketch uses `Serial` as the sensor stream, so you do not get USB debug output.
- On boards with a separate debug stream, upload the sketch and open Serial Monitor at `115200`.
