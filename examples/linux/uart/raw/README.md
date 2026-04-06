# Linux UART Raw

This directory contains the smallest Linux UART line-ownership example.

## Example

- `raw_command.c`: send one raw command, read the resulting UART line, and print the raw response

## Use It

1. Build with `cmake --preset host-linux-debug` and `cmake --build --preset host-linux-debug --parallel`.
2. Run `./build/host-linux-debug/ezo_linux_uart_raw_command_example [device_path] [baud] [command]`.
3. Start with a safe command such as `i` on a known UART path, usually `/dev/ttyUSB0` at `9600`.

## Notes

- This example stays close to the transport layer and does not hide UART line ownership.
- Use `../commissioning/` next if you need bootstrap, identity, or readiness checks.
