# Linux UART Typed Reads

These examples do one typed read per product over Linux UART with explicit response-code bootstrap and any required state query up front.

## Build

1. Run `cmake --preset host-linux-debug`.
2. Run `cmake --build --preset host-linux-debug --parallel`.

## Use It

- Pick the `read_<product>.c` file for the circuit you already know is attached.
- Run the matching binary from `build/host-linux-debug/`; the binary name matches the source stem with the `ezo_linux_uart_..._example` prefix.
- If transport or product is still uncertain, start with `../commissioning/`.

## Files

- `read_ph.c`
- `read_orp.c`
- `read_ec.c`
- `read_do.c`
- `read_rtd.c`
- `read_hum.c`
