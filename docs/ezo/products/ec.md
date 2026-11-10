# EZO-EC

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO conductivity family.

## Product Summary

- default transport mode is UART
- default I2C address is `100` (`0x64`)
- typical one-shot read timing is about `600 ms`
- supports temperature compensation
- supports multi-point calibration
- supports configurable probe characteristics and output fields

## Measurement Model

EC is the clearest example of why the repo should not pretend every EZO product returns one number.

The device can report multiple related values, including:

- conductivity
- total dissolved solids
- salinity
- specific gravity

When more than one field is enabled, vendor documentation defines a fixed output order. The current EC helper therefore parses an explicit CSV schema rather than calling `ezo_parse_double()` on the whole payload.

## Shared Command Families Present

The EC family includes the common acquisition, calibration, identity, control, and protocol-switching families, plus calibration export and import.

## Product-Specific Features

### Probe Configuration

The EC product exposes probe `K` configuration. That configuration changes how the device should be interpreted in an application and belongs in the current typed helper layer.

### Temperature Compensation

Temperature input directly affects readings and should be modeled as part of the product workflow, not as an afterthought.

### TDS Conversion Factor

The device can change its TDS conversion factor. That is not a transport concern; it is product behavior with application-level impact.

### Output Selection

The device can enable or disable its output fields independently. This changes the payload shape and is the strongest reason not to treat EC as a generic "read one value" device.

Vendor output-config replies also use an alternate query prefix shape: the documented response is `?,O,...` rather than the more common `?Prefix,...` form used by many other commands. The typed EC parser accepts that vendor-shaped response directly.

## Calibration Procedure

### Calibration Model

EC calibration is staged and order-sensitive:

- dry calibration must happen first
- dry plus one wet point gives a narrower-band calibration
- dry plus low and high wet points gives the broadest supported range

The command surface is:

- `Cal,dry`
- `Cal,n`
- `Cal,low,n`
- `Cal,high,n`
- `Cal,clear`
- `Cal,?`

`Cal,?` reports:

- `?CAL,0` = not calibrated
- `?CAL,1` = dry plus one wet point
- `?CAL,2` = dry plus low and high wet points

Plan on about `600 ms` for `Cal,dry`, `Cal,n`, `Cal,low,n`, and `Cal,high,n`, and about `300 ms` for `Cal,clear` and `Cal,?`.

### Before You Begin

- Perform the dry calibration first, even if the probe already reads `0.00` in air.
- Set the probe `K` value before calibration if you are not using the default `K 1.0`.
- Leave temperature compensation at the default `25 C` during calibration.
- If the solution temperature is about `5 C` or more away from `25 C`, calibrate to the bottle-corrected conductivity value for that temperature instead of the nominal `25 C` value.
- Use fresh solution in a separate cup rather than in the bottle.
- Shake the probe gently to remove trapped air before each wet calibration point.
- If the probe reads `0.00` in solution, stop and inspect the connection. The vendor guidance is explicit that you cannot calibrate EC to zero conductivity.

### Recommended Point Sets

The vendor recommendation depends on the probe `K` value:

- `K 0.1`: low `84 uS`, high `1413 uS`
- `K 1.0`: low `12880 uS`, high `80000 uS`
- `K 10`: low `12880 uS`, high `150000 uS`

You can use other certified values, but the maintained examples and the vendor guidance are built around the standard points above.

### Step-By-Step Procedure

#### Step 1: Set Probe Type

If the hardware is not `K 1.0`, set the correct `K` value before calibrating. Calibration data is only meaningful relative to the configured probe type.

#### Step 2: Dry Calibration

1. Keep the probe dry and in air.
2. Issue `Cal,dry`.
3. Treat this as mandatory even if the pre-calibration reading already looks like zero.

#### Step 3A: Narrower-Band Calibration

Use this when the operating range is narrow and well known:

1. Complete `Cal,dry`.
2. Place the probe in a known conductivity solution.
3. Remove trapped air and wait until the reading stabilizes.
4. Issue `Cal,n` using the conductivity value of that solution.
5. Confirm that the reading moves onto the target value.

#### Step 3B: Broad-Range Calibration

Use this when you need the broadest supported range:

1. Complete `Cal,dry`.
2. Place the probe in the low-point solution, remove trapped air, wait for stabilization, and issue `Cal,low,n`.
3. Do not expect the reading to jump immediately after the low-point command; the vendor material explicitly shows this point as an internal anchor rather than an immediate visible correction.
4. Rinse the probe.
5. Place the probe in the high-point solution, remove trapped air, wait for stabilization, and issue `Cal,high,n`.
6. Confirm that the reading now moves onto the high-point target.

### What "Stabilized" Means

EC must not be calibrated on a moving value. The vendor guidance explicitly warns that blind calibration produces drifting post-calibration readings. Stabilized means the value has flattened and is no longer materially walking toward the final number.

### Recalibration Expectations

The vendor position here is stronger than for most sensor types: a properly calibrated conductivity probe usually does not need routine recalibration. Recalibration is more likely after things that change the electrical characteristics of the system, such as changing cable length or moving the probe and circuit into a different machine or electrical environment.

## Firmware Caveats

Read the circuit's firmware with `i` before relying on a feature that arrived late. These versions come from the vendor's firmware changelog; none of them has been checked on a circuit.

Minimum firmware for the helpers that need one:

- protocol lock: `1.95`
- find, and calibration export and import: `2.10`
- read with temperature compensation (`RT`): `2.13`
- probe `K` values down to `0.01`: `2.14`

Known issues below the version that fixed them:

- below `2.12`: a circuit could wake from sleep at a different I2C address; see [`../common/i2c.md`](../common/i2c.md#sleep-over-i2c). `2.17` lists further sleep improvements
- below `2.11`: `Sleep` could draw excessive current
- below `2.15`: output did not always round to `0`
- below `1.96`: losing power 900 ms into startup could erase the EEPROM

## Timing Notes

The current repo read fallback of `1000 ms` is conservative for EC reads. Product-aware callers can usually wait less than that, but exact timing should still be treated as command-specific for calibration and control operations.

## Code Implications

The current typed EC module owns:

- a typed output parser
- output-enable configuration
- probe `K` configuration
- temperature-compensation helpers
- calibration query/set/clear helpers

The EC calibration-status query is vendor-quirky in current materials: examples use `?CAL,...` uppercase. The parser accepts that documented form rather than assuming only `?Cal,...`.

That layer sits above the transport APIs rather than complicating `src/ezo_i2c.*` or `src/ezo_uart.*`.

## Repo Entry Points

- Linux staged examples: `examples/linux/i2c/advanced/ec_calibration.c` and `examples/linux/uart/advanced/ec_calibration.c`
- Arduino staged example: `examples/arduino/i2c/advanced/ec_calibration/ec_calibration.ino`
- Cross-device compensation examples: `examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c`, `examples/linux/uart/advanced/ec_temp_comp_from_rtd.c`, and the matching Arduino I2C example
- Calibration transfer helpers: `src/ezo_calibration_transfer.h`
