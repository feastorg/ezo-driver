# Linux UART Advanced

These examples cover stateful UART workflows: calibration, compensation, calibration transfer, shared control, and transport switching.

## Build

1. Run `cmake --preset host-linux-debug`.
2. Run `cmake --build --preset host-linux-debug --parallel`.

## Use It

- Calibration examples stage one step at a time and only change device state when `--apply` is present.
- Cross-device compensation examples inspect first and only apply source values when `--apply` is present.
- Shared control, transport, and calibration-transfer examples inspect first and only mutate state when `--apply` is present.
- Run the matching binary from `build/host-linux-debug/`; the binary name matches the source stem with the `ezo_linux_uart_..._example` prefix.

## Example Families

- Calibration: `ph_calibration.c`, `orp_calibration.c`, `ec_calibration.c`, `do_calibration.c`, `rtd_calibration.c`, `hum_temperature_calibration.c`
- Product workflows: `ph_workflow.c`, `orp_workflow.c`, `ec_workflow.c`, `do_workflow.c`, `rtd_workflow.c`, `hum_workflow.c`
- Compensation chains: `ph_temp_comp_from_rtd.c`, `ec_temp_comp_from_rtd.c`, `do_temp_comp_from_rtd.c`, `do_salinity_comp_from_ec.c`, `do_full_compensation_chain.c`
- Shared control and admin: `control_workflow.c`, `transport_switch.c`, `calibration_transfer.c`
