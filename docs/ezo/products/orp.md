# EZO-ORP

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO ORP family.

## Product Summary

- measures oxidation-reduction potential as a single scalar reading in millivolts
- default transport mode is UART
- default I2C address is `98` (`0x62`)
- typical read cadence is about one reading per second
- temperature compensation is not part of the product model
- calibration is single-point

## Measurement Model

ORP is structurally simple for the driver:

- normal reads return one scalar value
- there is no configurable multi-output payload
- there is no temperature-compensated read family to model

That makes ORP one of the cleanest fits for the current baseline API surface.

## Shared Command Families Present

The ORP family includes the common acquisition, calibration, identity, control, and protocol-switching families.

## Product-Specific Features

The main ORP-specific extension is an extended-range toggle.

That matters because it changes the valid measurement envelope without changing the transport framing. The current typed helper models range state explicitly when it exposes this feature.

## Calibration Procedure

### Calibration Model

ORP uses a single-point calibration model. The circuit can be calibrated to any known ORP reference value, but the vendor guidance for first-time setup is to use a `225 mV` reference solution.

The command surface is:

- `Cal,n`
- `Cal,clear`
- `Cal,?`

`Cal,?` reports:

- `?Cal,0` = not calibrated
- `?Cal,1` = calibrated

Plan on about `900 ms` for `Cal,n` and about `300 ms` for `Cal,clear` and `Cal,?`.

### Before You Begin

- Rinse the probe before placing it in the calibration solution.
- The easiest workflow is UART mode with continuous readings, or an I2C workflow that repeatedly requests readings so you can actually watch the stabilization.
- First-time setup should use the standard `225 mV` solution unless you have a specific engineering reason to use another certified reference.
- Always watch live readings and calibrate only after they stabilize.

### Step-By-Step Procedure

1. Rinse the ORP probe and place it into a known calibration solution, typically `225 mV`.
2. Wait until the reading has stopped materially trending. Vendor guidance treats roughly `10` to `60` seconds as normal, with small movement between adjacent readings still acceptable.
3. Issue `Cal,225` or the equivalent `Cal,<known reference>` for your chosen solution.
4. Confirm the device acknowledges the command and that subsequent readings settle at the reference value.

### What "Stabilized" Means

ORP is another product where blind calibration fails badly. Do not calibrate on a moving value. Stabilized means the measurement has flattened out and repeated reads are clustering near one value rather than continuing to walk toward it.

### Verification And Troubleshooting

After calibration, the reading should settle near the reference solution value. If it does not:

- clear calibration and retry with fresh solution
- check for electrical noise before blaming the probe

The vendor notes that electrical noise often shows up as fast fluctuations or a steady offset. A simple check is to isolate the ORP probe in a separate cup of water and see whether the reading stabilizes normally there.

### Calibration Frequency

Vendor guidance is at least annual calibration. If the application spends long periods near the extreme ends of the ORP scale, recalibration may need to happen more often.

### Repo Entry Points

- Linux staged examples: `examples/linux/i2c/advanced/orp_calibration.c` and `examples/linux/uart/advanced/orp_calibration.c`
- Arduino staged example: `examples/arduino/i2c/advanced/orp_calibration/orp_calibration.ino`
- Calibration transfer helpers: `src/ezo_calibration_transfer.h`

## Firmware Caveats

Read the circuit's firmware with `i` before relying on a feature that arrived late. These versions come from the vendor's firmware changelog; none of them has been checked on a circuit.

Minimum firmware for the helpers that need one:

- protocol lock: `1.95`
- find, and calibration export and import: `2.10`
- extended range (`ORPext`): `2.14`

Known issues below the version that fixed them:

- below `2.13`: a bug in I2C timing and sleep, the same entry as pH `2.16`; see [`../common/i2c.md`](../common/i2c.md#sleep-over-i2c)
- below `2.11`: a cleared calibration could come back after a restart
- below `1.97`: a bug in `Cal,clear`
- below `1.96`: losing power 900 ms into startup could erase the EEPROM

## Timing Notes

The broad repo read hint of `1000 ms` matches the product summary well enough for ORP. That makes ORP a good example of where the generic timing hint is conservative but not misleading.

## Code Implications

The current typed ORP helper stays small:

- plain read
- calibration query and control
- extended-range query and control

ORP does not need a multi-value parser layer, which keeps it separate from EC, DO, and HUM in future API design.
