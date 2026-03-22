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

## Structure

- `examples/linux/i2c/raw/`: raw Linux I2C transport examples
- `examples/linux/i2c/commissioning/`: identity and readiness checks for known I2C devices
- `examples/linux/i2c/typed/`: one simple typed read per supported product
- `examples/linux/i2c/advanced/`: safe-by-default stateful workflows
- `examples/linux/uart/raw/`: raw Linux UART transport examples with explicit line ownership
- `examples/linux/uart/commissioning/`: UART bootstrap, identity, and readiness checks
- `examples/linux/uart/typed/`: one simple typed read per supported product with explicit response-code bootstrap
- `examples/linux/uart/advanced/`: safe-by-default UART workflows
- `examples/arduino/i2c/`: curated I2C smoke, inspect, pH read, and D.O. read sketches
- `examples/arduino/uart/`: curated UART smoke, inspect, pH read, and D.O. read sketches

## Safety Model

- Advanced examples inspect first and only apply changes when `--apply` is present.
- Calibration-transfer examples never import anything unless `--apply` and `--payload=...` are both present.
- UART typed and advanced examples explicitly bootstrap response-code mode before assuming `DATA + *OK` success sequences.
- Raw examples stay close to the transport layer and do not hide line ownership or synchronization.

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

- I2C pH workflow: [`examples/linux/i2c/advanced/ph_workflow.c`](../examples/linux/i2c/advanced/ph_workflow.c)
- I2C D.O. workflow: [`examples/linux/i2c/advanced/do_workflow.c`](../examples/linux/i2c/advanced/do_workflow.c)
- I2C RTD workflow: [`examples/linux/i2c/advanced/rtd_workflow.c`](../examples/linux/i2c/advanced/rtd_workflow.c)
- I2C calibration transfer: [`examples/linux/i2c/advanced/calibration_transfer.c`](../examples/linux/i2c/advanced/calibration_transfer.c)
- UART pH workflow: [`examples/linux/uart/advanced/ph_workflow.c`](../examples/linux/uart/advanced/ph_workflow.c)
- UART D.O. workflow: [`examples/linux/uart/advanced/do_workflow.c`](../examples/linux/uart/advanced/do_workflow.c)
- UART RTD workflow: [`examples/linux/uart/advanced/rtd_workflow.c`](../examples/linux/uart/advanced/rtd_workflow.c)
- UART calibration transfer: [`examples/linux/uart/advanced/calibration_transfer.c`](../examples/linux/uart/advanced/calibration_transfer.c)

## Arduino Curated Matrix

- I2C raw smoke: [`examples/arduino/i2c/raw/smoke/smoke.ino`](../examples/arduino/i2c/raw/smoke/smoke.ino)
- I2C inspect device: [`examples/arduino/i2c/commissioning/inspect_device/inspect_device.ino`](../examples/arduino/i2c/commissioning/inspect_device/inspect_device.ino)
- I2C typed pH read: [`examples/arduino/i2c/typed/read_ph/read_ph.ino`](../examples/arduino/i2c/typed/read_ph/read_ph.ino)
- I2C typed D.O. read: [`examples/arduino/i2c/typed/read_do/read_do.ino`](../examples/arduino/i2c/typed/read_do/read_do.ino)
- UART raw smoke: [`examples/arduino/uart/raw/smoke/smoke.ino`](../examples/arduino/uart/raw/smoke/smoke.ino)
- UART inspect device: [`examples/arduino/uart/commissioning/inspect_device/inspect_device.ino`](../examples/arduino/uart/commissioning/inspect_device/inspect_device.ino)
- UART typed pH read: [`examples/arduino/uart/typed/read_ph/read_ph.ino`](../examples/arduino/uart/typed/read_ph/read_ph.ino)
- UART typed D.O. read: [`examples/arduino/uart/typed/read_do/read_do.ino`](../examples/arduino/uart/typed/read_do/read_do.ino)

Linux is the full reference surface. Arduino stays intentionally smaller so the sketches remain readable and CI remains cheap enough to run across the current board set.

## Notes

- I2C examples take `device_path` first and `address` second when those arguments are provided.
- UART examples take `device_path` first and `baud` second when those arguments are provided.
- Product-specific flags, when present, come after the transport arguments.
- The calibration-transfer examples default to the pH product family for convenience. If you use another product, pass the correct transport/address combination yourself and override the product with `--product=...`.
