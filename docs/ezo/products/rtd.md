# EZO-RTD

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO RTD family.

## Product Summary

- measures temperature as a single scalar reading
- default transport mode is UART
- default I2C address is `102` (`0x66`)
- summary material describes roughly one-second cadence, while protocol-specific flows can be faster
- supports single-point calibration
- supports selectable output scale
- includes an onboard data logger and memory recall surface

## Measurement Model

RTD returns one temperature value at a time, but that value is not unit-invariant. The device can express readings in Celsius, Kelvin, or Fahrenheit.

That means a typed helper should either:

- preserve unit state explicitly
- or normalize units in a clearly documented way

## Shared Command Families Present

The RTD family includes the common acquisition, calibration, identity, control, and protocol-switching families, plus calibration export and import.

## Product-Specific Features

### Temperature Scale

Scale selection changes the meaning of ordinary read results even though the payload remains scalar text.

### Data Logger And Memory

RTD is the outlier in the current product set because it includes onboard logging and stored-reading recall. Those commands do not belong in a generic EZO abstraction.

The recall surface is split into:

- sequential `M` history reads with explicit indices in the payload
- bulk `M,all` history recall as a CSV list of stored temperatures
- memory-clear control

## Calibration Procedure

### Calibration Model

RTD uses a single-point offset calibration:

- `Cal,t`
- `Cal,clear`
- `Cal,?`

`Cal,?` reports:

- `?Cal,0` = not calibrated
- `?Cal,1` = calibrated

Plan on about `600 ms` for `Cal,t` and about `300 ms` for `Cal,clear` and `Cal,?`.

### Before You Begin

- The easiest workflow is UART mode with live continuous readings, or an I2C workflow that repeatedly requests readings so you can watch the value settle.
- Switching protocols after calibration does not affect the stored RTD calibration.
- If you use the common boiling-water method, use purified or distilled water.
- Do not assume the boiling point is always exactly `100.0 C`; account for local elevation or pressure.
- If you need a high-accuracy point other than the local boiling point, use a dry block calibrator rather than guessing from ambient conditions.

### Step-By-Step Procedure

1. Place the RTD probe in a known temperature environment.
2. Wait until the reading has fully stabilized.
3. Issue `Cal,<known temperature>`.
4. Confirm that subsequent readings settle on the reference temperature.

The simplest field method in the vendor material is boiling-water calibration using the correct local boiling point. At sea level that is `100.0 C`, but the setpoint should be adjusted if the actual boiling point differs at your elevation.

### What "Stabilized" Means

RTD follows the same "never do a blind calibration" rule as the other products. If the command is sent while the reading is still moving, the calibrated result will drift instead of landing cleanly on the target temperature.

### Calibration Interval

Vendor guidance is unusually explicit here: recalibration is generally recommended about every three years rather than on every maintenance cycle.

## Firmware Caveats

Read the circuit's firmware with `i` before relying on a feature that arrived late. These versions come from the vendor's firmware changelog; none of them has been checked on a circuit.

Minimum firmware for the helpers that need one:

- protocol lock: `1.02`
- find, and calibration export and import: `2.10`
- the `*OK` and `Baud` commands: `2.01`; older firmware named them `response` and `Serial`

Known issues below the version that fixed them:

- below `2.15`: the changelog lists sleep improvements at `2.15`
- below `2.11`: the first reading after boot could be `-1024`
- below `2.02`: calibration did not accept floating-point values
- below `1.11`: a blank name could produce garbage output
- below `1.03`: losing power 900 ms into startup could erase the EEPROM

## Timing Notes

The generic repo read hint is conservative for RTD and broadly safe across the vendor timing views. Logger and memory operations should still be treated as their own product behaviors rather than folded into the normal read path.

## Code Implications

The current typed RTD module now groups:

- read and calibration helpers
- unit selection
- logger control
- sequential memory recall, bulk memory recall, and clear

RTD is still simpler than EC, DO, and HUM from a parsing perspective, but broader from a device-feature perspective.

## Repo Entry Points

- Linux staged examples: `examples/linux/i2c/advanced/rtd_calibration.c` and `examples/linux/uart/advanced/rtd_calibration.c`
- Arduino staged example: `examples/arduino/i2c/advanced/rtd_calibration/rtd_calibration.ino`
- Calibration transfer helpers: `src/ezo_calibration_transfer.h`
