# Linux Example Support

This directory contains shared support code used by the Linux example binaries. These files are not meant to be run directly.

## Files

- `example_base.c`: shared timing, sleep, and error-print helpers
- `example_control.c`: shared device-info and shared-control queries
- `example_i2c.c`: Linux I2C option parsing and session helpers
- `example_uart.c`: Linux UART option parsing and response-code bootstrap helpers
- `example_products.c`: shared product-formatting helpers for typed and advanced examples

## Use It

1. Build the repo with `cmake --preset host-linux-debug`.
2. Build the example binaries with `cmake --build --preset host-linux-debug --parallel`.
3. Use the sources in this directory only as references when extending the maintained Linux examples.

## Notes

- The runnable examples live under `examples/linux/i2c/*` and `examples/linux/uart/*`.
- The full chooser is in [`docs/examples.md`](../../../docs/examples.md).
