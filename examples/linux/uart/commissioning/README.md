# Linux UART Commissioning

These examples identify a UART-connected device, bootstrap response-code mode when needed, and print readiness state before you move on to typed reads or calibration.

## Build

1. Run `cmake --preset host-linux-debug`.
2. Run `cmake --build --preset host-linux-debug --parallel`.

## Use It

- Start with `inspect_device.c` if the product or current state is still unknown.
- Use `readiness_check.c` when you need product-specific setup signals such as calibration, output masks, or logger state.
- Run the matching binary from `build/host-linux-debug/`; the binary name matches the source stem with the `ezo_linux_uart_..._example` prefix.

## Files

- `inspect_device.c`
- `readiness_check.c`
