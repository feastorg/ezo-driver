# Examples Guide

This repo now treats examples as a staged ladder instead of a flat pile of one-off files.

Use this page as the chooser.

## Start Here

- If transport is unknown, start with [`examples/linux/uart/commissioning/inspect_device.c`](../examples/linux/uart/commissioning/inspect_device.c).
  Across the currently documented products, UART is the shipping default mode.
- If transport is known but setup state is not, use the matching readiness check:
  [`examples/linux/i2c/commissioning/readiness_check.c`](../examples/linux/i2c/commissioning/readiness_check.c)
  or [`examples/linux/uart/commissioning/readiness_check.c`](../examples/linux/uart/commissioning/readiness_check.c)
- If product and transport are already known, jump to the matching typed read example under `examples/linux/<transport>/typed/`.
- If you need to understand the raw transport contract before the typed helpers, start with:
  [`examples/linux/i2c/raw/raw_command.c`](../examples/linux/i2c/raw/raw_command.c)
  or [`examples/linux/uart/raw/raw_command.c`](../examples/linux/uart/raw/raw_command.c)
- If the device is uncalibrated or configuration-sensitive, read the matching readiness example first and then move to the advanced workflow for that capability class.
- If you need a vendor-style staged calibration flow, start with the matching `<product>_calibration.c` example under `examples/linux/<transport>/advanced/`.
- If you need a cross-device compensation chain, start with:
  [`examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c`](../examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c),
  [`examples/linux/i2c/advanced/do_salinity_comp_from_ec.c`](../examples/linux/i2c/advanced/do_salinity_comp_from_ec.c),
  [`examples/linux/uart/advanced/ec_temp_comp_from_rtd.c`](../examples/linux/uart/advanced/ec_temp_comp_from_rtd.c),
  or [`examples/linux/uart/advanced/do_salinity_comp_from_ec.c`](../examples/linux/uart/advanced/do_salinity_comp_from_ec.c).
- If you need shared admin or mode-switch behavior, start with:
  [`examples/linux/i2c/advanced/control_workflow.c`](../examples/linux/i2c/advanced/control_workflow.c),
  [`examples/linux/i2c/advanced/transport_switch.c`](../examples/linux/i2c/advanced/transport_switch.c),
  [`examples/linux/uart/advanced/control_workflow.c`](../examples/linux/uart/advanced/control_workflow.c),
  or [`examples/linux/uart/advanced/transport_switch.c`](../examples/linux/uart/advanced/transport_switch.c).

## Structure

- `examples/linux/i2c/raw/`: raw Linux I2C transport examples
- `examples/linux/i2c/commissioning/`: identity and readiness checks for known I2C devices
- `examples/linux/i2c/typed/`: one simple typed read per supported product
- `examples/linux/i2c/advanced/`: safe-by-default stateful workflows
- `examples/linux/uart/raw/`: raw Linux UART transport examples with explicit line ownership
- `examples/linux/uart/commissioning/`: UART bootstrap, identity, and readiness checks
- `examples/linux/uart/typed/`: one simple typed read per supported product with explicit response-code bootstrap
- `examples/linux/uart/advanced/`: safe-by-default UART workflows
- `examples/arduino/i2c/advanced/`: curated hardware-facing I2C compensation sketch
- `examples/arduino/uart/advanced/`: curated hardware-topology-specific UART routing sketch
- `examples/arduino/i2c/`: curated I2C smoke, inspect, pH read, and D.O. read sketches
- `examples/arduino/uart/`: curated UART smoke, inspect, pH read, and D.O. read sketches

## Safety Model

- Advanced examples inspect first and only apply changes when `--apply` is present.
- Advanced calibration examples use explicit `--step=` staging and bounded preview loops instead of open-ended polling or shells.
- Calibration-transfer examples never import anything unless `--apply` and `--payload=...` are both present.
- UART typed and advanced examples explicitly bootstrap response-code mode before assuming `DATA + *OK` success sequences.
- Raw examples stay close to the transport layer and do not hide line ownership or synchronization.
- Arduino advanced sketches stay explicit and hardware-facing; they do not reintroduce sequencer, shell, or scheduler abstractions.

## Linux Reference Matrix

### Raw

- I2C raw command: [`examples/linux/i2c/raw/raw_command.c`](../examples/linux/i2c/raw/raw_command.c)
- UART raw command: [`examples/linux/uart/raw/raw_command.c`](../examples/linux/uart/raw/raw_command.c)

### Commissioning

- I2C inspect device: [`examples/linux/i2c/commissioning/inspect_device.c`](../examples/linux/i2c/commissioning/inspect_device.c)
- I2C readiness check: [`examples/linux/i2c/commissioning/readiness_check.c`](../examples/linux/i2c/commissioning/readiness_check.c)
- UART inspect device: [`examples/linux/uart/commissioning/inspect_device.c`](../examples/linux/uart/commissioning/inspect_device.c)
- UART readiness check: [`examples/linux/uart/commissioning/readiness_check.c`](../examples/linux/uart/commissioning/readiness_check.c)

### Typed Reads

- I2C pH: [`examples/linux/i2c/typed/read_ph.c`](../examples/linux/i2c/typed/read_ph.c)
- I2C ORP: [`examples/linux/i2c/typed/read_orp.c`](../examples/linux/i2c/typed/read_orp.c)
- I2C EC: [`examples/linux/i2c/typed/read_ec.c`](../examples/linux/i2c/typed/read_ec.c)
- I2C D.O.: [`examples/linux/i2c/typed/read_do.c`](../examples/linux/i2c/typed/read_do.c)
- I2C RTD: [`examples/linux/i2c/typed/read_rtd.c`](../examples/linux/i2c/typed/read_rtd.c)
- I2C HUM: [`examples/linux/i2c/typed/read_hum.c`](../examples/linux/i2c/typed/read_hum.c)
- UART pH: [`examples/linux/uart/typed/read_ph.c`](../examples/linux/uart/typed/read_ph.c)
- UART ORP: [`examples/linux/uart/typed/read_orp.c`](../examples/linux/uart/typed/read_orp.c)
- UART EC: [`examples/linux/uart/typed/read_ec.c`](../examples/linux/uart/typed/read_ec.c)
- UART D.O.: [`examples/linux/uart/typed/read_do.c`](../examples/linux/uart/typed/read_do.c)
- UART RTD: [`examples/linux/uart/typed/read_rtd.c`](../examples/linux/uart/typed/read_rtd.c)
- UART HUM: [`examples/linux/uart/typed/read_hum.c`](../examples/linux/uart/typed/read_hum.c)

### Advanced

#### Calibration

- I2C pH calibration: [`examples/linux/i2c/advanced/ph_calibration.c`](../examples/linux/i2c/advanced/ph_calibration.c)
- I2C ORP calibration: [`examples/linux/i2c/advanced/orp_calibration.c`](../examples/linux/i2c/advanced/orp_calibration.c)
- I2C EC calibration: [`examples/linux/i2c/advanced/ec_calibration.c`](../examples/linux/i2c/advanced/ec_calibration.c)
- I2C D.O. calibration: [`examples/linux/i2c/advanced/do_calibration.c`](../examples/linux/i2c/advanced/do_calibration.c)
- I2C RTD calibration: [`examples/linux/i2c/advanced/rtd_calibration.c`](../examples/linux/i2c/advanced/rtd_calibration.c)
- I2C HUM temperature calibration: [`examples/linux/i2c/advanced/hum_temperature_calibration.c`](../examples/linux/i2c/advanced/hum_temperature_calibration.c)
- UART pH calibration: [`examples/linux/uart/advanced/ph_calibration.c`](../examples/linux/uart/advanced/ph_calibration.c)
- UART ORP calibration: [`examples/linux/uart/advanced/orp_calibration.c`](../examples/linux/uart/advanced/orp_calibration.c)
- UART EC calibration: [`examples/linux/uart/advanced/ec_calibration.c`](../examples/linux/uart/advanced/ec_calibration.c)
- UART D.O. calibration: [`examples/linux/uart/advanced/do_calibration.c`](../examples/linux/uart/advanced/do_calibration.c)
- UART RTD calibration: [`examples/linux/uart/advanced/rtd_calibration.c`](../examples/linux/uart/advanced/rtd_calibration.c)
- UART HUM temperature calibration: [`examples/linux/uart/advanced/hum_temperature_calibration.c`](../examples/linux/uart/advanced/hum_temperature_calibration.c)

#### Product Workflows

- I2C pH workflow: [`examples/linux/i2c/advanced/ph_workflow.c`](../examples/linux/i2c/advanced/ph_workflow.c)
- I2C ORP workflow: [`examples/linux/i2c/advanced/orp_workflow.c`](../examples/linux/i2c/advanced/orp_workflow.c)
- I2C EC workflow: [`examples/linux/i2c/advanced/ec_workflow.c`](../examples/linux/i2c/advanced/ec_workflow.c)
- I2C D.O. workflow: [`examples/linux/i2c/advanced/do_workflow.c`](../examples/linux/i2c/advanced/do_workflow.c)
- I2C RTD workflow: [`examples/linux/i2c/advanced/rtd_workflow.c`](../examples/linux/i2c/advanced/rtd_workflow.c)
- I2C HUM workflow: [`examples/linux/i2c/advanced/hum_workflow.c`](../examples/linux/i2c/advanced/hum_workflow.c)
- I2C calibration transfer: [`examples/linux/i2c/advanced/calibration_transfer.c`](../examples/linux/i2c/advanced/calibration_transfer.c)
- UART pH workflow: [`examples/linux/uart/advanced/ph_workflow.c`](../examples/linux/uart/advanced/ph_workflow.c)
- UART ORP workflow: [`examples/linux/uart/advanced/orp_workflow.c`](../examples/linux/uart/advanced/orp_workflow.c)
- UART EC workflow: [`examples/linux/uart/advanced/ec_workflow.c`](../examples/linux/uart/advanced/ec_workflow.c)
- UART D.O. workflow: [`examples/linux/uart/advanced/do_workflow.c`](../examples/linux/uart/advanced/do_workflow.c)
- UART RTD workflow: [`examples/linux/uart/advanced/rtd_workflow.c`](../examples/linux/uart/advanced/rtd_workflow.c)
- UART HUM workflow: [`examples/linux/uart/advanced/hum_workflow.c`](../examples/linux/uart/advanced/hum_workflow.c)
- UART calibration transfer: [`examples/linux/uart/advanced/calibration_transfer.c`](../examples/linux/uart/advanced/calibration_transfer.c)

#### Cross-Device Compensation

- I2C RTD -> pH temperature compensation: [`examples/linux/i2c/advanced/ph_temp_comp_from_rtd.c`](../examples/linux/i2c/advanced/ph_temp_comp_from_rtd.c)
- I2C RTD -> EC temperature compensation: [`examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c`](../examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c)
- I2C RTD -> D.O. temperature compensation: [`examples/linux/i2c/advanced/do_temp_comp_from_rtd.c`](../examples/linux/i2c/advanced/do_temp_comp_from_rtd.c)
- I2C EC -> D.O. salinity compensation: [`examples/linux/i2c/advanced/do_salinity_comp_from_ec.c`](../examples/linux/i2c/advanced/do_salinity_comp_from_ec.c)
- I2C full D.O. chain: [`examples/linux/i2c/advanced/do_full_compensation_chain.c`](../examples/linux/i2c/advanced/do_full_compensation_chain.c)
- UART RTD -> pH temperature compensation: [`examples/linux/uart/advanced/ph_temp_comp_from_rtd.c`](../examples/linux/uart/advanced/ph_temp_comp_from_rtd.c)
- UART RTD -> EC temperature compensation: [`examples/linux/uart/advanced/ec_temp_comp_from_rtd.c`](../examples/linux/uart/advanced/ec_temp_comp_from_rtd.c)
- UART RTD -> D.O. temperature compensation: [`examples/linux/uart/advanced/do_temp_comp_from_rtd.c`](../examples/linux/uart/advanced/do_temp_comp_from_rtd.c)
- UART EC -> D.O. salinity compensation: [`examples/linux/uart/advanced/do_salinity_comp_from_ec.c`](../examples/linux/uart/advanced/do_salinity_comp_from_ec.c)
- UART full D.O. chain: [`examples/linux/uart/advanced/do_full_compensation_chain.c`](../examples/linux/uart/advanced/do_full_compensation_chain.c)

#### Shared Control and Transport

- I2C control workflow: [`examples/linux/i2c/advanced/control_workflow.c`](../examples/linux/i2c/advanced/control_workflow.c)
- I2C transport switch: [`examples/linux/i2c/advanced/transport_switch.c`](../examples/linux/i2c/advanced/transport_switch.c)
- UART control workflow: [`examples/linux/uart/advanced/control_workflow.c`](../examples/linux/uart/advanced/control_workflow.c)
- UART transport switch: [`examples/linux/uart/advanced/transport_switch.c`](../examples/linux/uart/advanced/transport_switch.c)

## Arduino Curated Matrix

- I2C raw smoke: [`examples/arduino/i2c/raw/smoke/smoke.ino`](../examples/arduino/i2c/raw/smoke/smoke.ino)
- I2C inspect device: [`examples/arduino/i2c/commissioning/inspect_device/inspect_device.ino`](../examples/arduino/i2c/commissioning/inspect_device/inspect_device.ino)
- I2C typed pH read: [`examples/arduino/i2c/typed/read_ph/read_ph.ino`](../examples/arduino/i2c/typed/read_ph/read_ph.ino)
- I2C typed D.O. read: [`examples/arduino/i2c/typed/read_do/read_do.ino`](../examples/arduino/i2c/typed/read_do/read_do.ino)
- I2C advanced EC temp compensation from RTD: [`examples/arduino/i2c/advanced/ec_temp_comp_from_rtd/ec_temp_comp_from_rtd.ino`](../examples/arduino/i2c/advanced/ec_temp_comp_from_rtd/ec_temp_comp_from_rtd.ino)
- UART raw smoke: [`examples/arduino/uart/raw/smoke/smoke.ino`](../examples/arduino/uart/raw/smoke/smoke.ino)
- UART inspect device: [`examples/arduino/uart/commissioning/inspect_device/inspect_device.ino`](../examples/arduino/uart/commissioning/inspect_device/inspect_device.ino)
- UART typed pH read: [`examples/arduino/uart/typed/read_ph/read_ph.ino`](../examples/arduino/uart/typed/read_ph/read_ph.ino)
- UART typed D.O. read: [`examples/arduino/uart/typed/read_do/read_do.ino`](../examples/arduino/uart/typed/read_do/read_do.ino)
- UART advanced multi-device router: [`examples/arduino/uart/advanced/multi_device_router/multi_device_router.ino`](../examples/arduino/uart/advanced/multi_device_router/multi_device_router.ino)

Linux is the full reference surface. Arduino stays intentionally smaller: smoke, inspect, simple typed reads, one cross-device I2C compensation sketch, and one hardware-topology-specific UART routing sketch.

## Legacy Mapping

The `_reference/` tree is retired. The table below records what survived as canonical guidance and what was intentionally dropped.

| Legacy family | Status | Canonical replacement or rationale |
| --- | --- | --- |
| `_reference/Ezo_I2c_lib/Examples/I2c_lib_examples/EC_parsing_and_DO_compensation` | replaced in this phase | [`examples/linux/i2c/advanced/do_salinity_comp_from_ec.c`](../examples/linux/i2c/advanced/do_salinity_comp_from_ec.c) and [`examples/linux/uart/advanced/do_salinity_comp_from_ec.c`](../examples/linux/uart/advanced/do_salinity_comp_from_ec.c) keep the explicit EC-to-D.O. compensation chain without sequencer scaffolding. |
| `_reference/Ezo_I2c_lib/Examples/I2c_lib_examples/temp_comp_example` and `temp_comp_rt_example` | replaced in this phase | [`examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c`](../examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c), [`examples/linux/uart/advanced/ec_temp_comp_from_rtd.c`](../examples/linux/uart/advanced/ec_temp_comp_from_rtd.c), and the Arduino I2C advanced sketch preserve RTD-driven EC compensation with explicit state ownership. |
| `_reference/Ezo_I2c_lib/Examples/I2c_lib_examples/humidity_parsing` | superseded | The typed HUM reads under `examples/linux/*/typed/read_hum.c` replace manual CSV destructuring with canonical typed helpers. |
| `_reference/Ezo_I2c_lib/Examples/I2c_lib_examples/I2c_read_mulitple_circuits` | superseded | The new cross-device advanced examples show explicit multi-device ownership without a sequencer abstraction. |
| `_reference/Ezo_I2c_lib/Examples/I2c_lib_examples/i2c_scan_and_comms_example` | retired | It is an interactive command-shell plus bus-discovery pattern, not canonical repo-level driver guidance. The repo now prefers explicit transport bring-up and commissioning examples instead of a baked-in console workflow. |
| `_reference/Ezo_I2c_lib/Examples/I2c_lib_examples/iot_cmd_sample_code` | retired | `iot_cmd` is a legacy shell framework. The canonical repo keeps transport-explicit examples and does not ship a command-console layer. |
| `_reference/Ezo_uart_lib/Examples/Serial_port_expander_example` | replaced in this phase | [`examples/arduino/uart/advanced/multi_device_router/multi_device_router.ino`](../examples/arduino/uart/advanced/multi_device_router/multi_device_router.ino) preserves the routed multi-device UART pattern without the old wrapper library or shell behavior. |
| `_reference/Ezo_uart_lib/Examples/Serial_port_expander_tempcomp_example` | replaced in this phase | The canonical router sketch plus the new compensation-chain examples cover the kept reusable behavior without port-expander-specific shell code. |
| `_reference/Ezo_uart_lib/Examples/Arduino_mega_example` and `ESP32_sample_code` | superseded | They were board-shaped variants of the routed UART pattern. The canonical router sketch replaces them with one maintained example and source-level board gating. |
| `_reference/Ezo_I2c_lib/Examples/IOT_kits/**` | retired | Wi-Fi kit application code is product-kit firmware, not portable driver guidance. |
| `_reference/Ezo_I2c_lib/Examples/Projects/**` | retired | Project application sketches are intentionally out of scope for the canonical driver repo. |
| `_reference/Ezo_I2c_lib/Examples/Products/**` | retired | Product-board app code is board-specific integration logic, not reusable library guidance. |
| `_reference/Ezo_I2c_lib/Examples/Sequencer_lib_examples/**` | retired | The sequencer examples demonstrate a legacy control framework that is antithetical to the current explicit library direction. |
| `_reference/Ezo_I2c_lib/*` and `_reference/Ezo_uart_lib/*` library sources | retired | The repo now ships one canonical library surface in `src/`; the legacy reference libraries are not a second maintained implementation. |

## Notes

- I2C examples take `device_path` first and `address` second when those arguments are provided.
- UART examples take `device_path` first and `baud` second when those arguments are provided.
- Product-specific flags, when present, come after the transport arguments.
- The calibration-transfer examples default to the pH product family for convenience. If you use another product, pass the correct transport/address combination yourself and override the product with `--product=...`.
