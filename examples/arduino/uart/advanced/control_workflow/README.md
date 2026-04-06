# Arduino UART Control Workflow

This sketch inspects shared UART control state such as name, LED, protocol lock, response-code mode, and optional admin operations like find, sleep, and factory reset.

## Use It

- AVR boards use `SoftwareSerial` on pins `10/11`; ESP32 uses `Serial1` on pins `16/17`; boards with `Serial1` use that port directly.
- Set any planned values such as `PLANNED_NAME`, `PLANNED_LED`, `PLANNED_PROTOCOL_LOCK`, or `PLANNED_RESPONSE_CODES`.
- Leave `APPLY_CHANGES=0` first and inspect the current state.
- Re-upload with `APPLY_CHANGES=1` only when you actually want to apply the selected control action.
