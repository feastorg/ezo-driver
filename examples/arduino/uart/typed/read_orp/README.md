# Arduino UART Read ORP

This sketch performs one typed ORP read over Arduino UART with explicit response-code bootstrap.

## Use It

- AVR boards use `SoftwareSerial` on pins `10/11`; ESP32 uses `Serial1` on pins `16/17`; boards with `Serial1` use that port directly.
- Upload the sketch and open Serial Monitor at `115200`.
- Move to `../../advanced/control_workflow/control_workflow.ino` if you need shared UART admin state.
