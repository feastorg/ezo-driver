# EZO-DO

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO dissolved-oxygen family.

## Product Summary

- default transport mode is UART
- default I2C address is `97` (`0x61`)
- typical one-shot read timing is about `600 ms`
- supports one-point and two-point calibration
- supports temperature, salinity, and atmospheric-pressure compensation
- can expose dissolved oxygen in more than one output form

## Measurement Model

DO is not just a scalar sensor with a single correction term.

The product can report dissolved oxygen as:

- mg/L
- percent saturation

The enabled output set affects payload shape, so callers should not assume one fixed-field response unless device configuration is already controlled by the application.

## Shared Command Families Present

The DO family includes the common acquisition, calibration, identity, control, and protocol-switching families, plus calibration export and import.

## Product-Specific Features

### Compensation Surface

DO has the broadest compensation surface among the current products:

- temperature
- salinity
- atmospheric pressure

That is why the current typed helper layer exposes these settings explicitly, because they shape both calibration practice and measurement interpretation.

### Output Selection

The product can enable or disable mg/L and percent-saturation outputs. As with EC and HUM, payload parsing belongs in a product-aware layer.

The documented output-config query reply uses the alternate vendor form `?,O,...` rather than the more common `?Prefix,...` shape. The typed DO parser accepts that response directly.

### Pressure Compensation Query Shape

The atmospheric-pressure query is another vendor oddball: the documented reply uses `?,P,...`. That shape differs from the simpler `?T,...` and `?S,...` query families, so the DO helper treats it as product-specific parsing instead of relying on the generic shared query helper.

## Calibration Procedure

### Calibration Model

DO supports:

- a high-point atmospheric calibration with `Cal`
- an optional low-point zero-oxygen calibration with `Cal,0`

The command surface is:

- `Cal`
- `Cal,0`
- `Cal,clear`
- `Cal,?`

`Cal,?` reports:

- `?Cal,0` = not calibrated
- `?Cal,1` = atmospheric point only
- `?Cal,2` = low point plus atmospheric point

Plan on about `1300 ms` for `Cal` and `Cal,0`, and about `300 ms` for `Cal,clear` and `Cal,?`.

### Before You Begin

- Verify the probe basically works before calibrating. Vendor guidance is to read in air first; readings above `10 mg/L` are treated as a healthy starting point, while readings below `5 mg/L` point to a probe issue that should be fixed first.
- Reset compensation to the default calibration values before you calibrate:
  - temperature `20 C`
  - atmospheric pressure `101 kPa`
  - salinity `0`
- Do not apply temperature, pressure, or salinity compensation until the base calibration is complete.
- Watch live readings and calibrate only after the values stabilize.

### Why The Default Compensation Matters

With the default compensation state, the atmospheric high-point calibration has a known expected value of about `9.09 mg/L`. Once you add custom compensation, the "correct" post-calibration value becomes application-specific and much harder to verify.

### Step-By-Step Procedure

#### Step 1: Low-Point Calibration

1. Place the probe in a zero dissolved oxygen solution.
2. Stir or move the probe enough to release trapped air, since trapped air can push the reading artificially high.
3. Continue reading until the value reaches zero, or until you have waited as long as the vendor guidance allows for your situation.
4. Issue `Cal,0`.

The vendor guidance is unusually specific here:

- a new probe usually reaches zero in a few minutes
- if it has not reached zero after about `4 hours`, calibrate to zero anyway
- after a fresh electrolyte refill, reaching zero may take several hours
- if it still has not reached zero after about `12 hours` in that post-refill case, calibrate to zero anyway

#### Step 2: High-Point Atmospheric Calibration

1. Expose the probe to air.
2. Wait until the reading stabilizes; vendor guidance treats roughly `5` to `30` seconds as typical.
3. Issue `Cal`.
4. With default compensation values still in place, confirm that the final reading settles around `9.09` to `9.1x mg/L`.

### Advanced Temperature Recalibration

Probe temperature recalibration is not the same thing as changing the temperature-compensation setting. The vendor guidance is:

1. complete the normal calibration first
2. move the probe to its real operating temperature
3. let it acclimate for about `10 minutes`
4. rerun the atmospheric `Cal` step at that operating temperature

This is only needed when the operating temperature is materially different from the temperature where the probe was last calibrated.

### What "Stabilized" Means

DO calibration is extremely sensitive to blind calibration. If the reading is still walking when you issue `Cal` or `Cal,0`, the post-calibration output will drift instead of staying fixed.

## Firmware Caveats

Read the circuit's firmware with `i` before relying on a feature that arrived late. These versions come from the vendor's firmware changelog; only Sleep and wake on `2.17` has been checked on a circuit.

Minimum firmware for the helpers that need one:

- protocol lock: `1.95`
- find, and calibration export and import: `2.10`
- read with temperature compensation (`RT`): `2.13`

Known issues below the version that fixed them:

- below `2.16`: the changelog lists sleep improvements at `2.16`. A circuit reporting `2.17`, newer than the last changelog entry, woke normally on the first command ([bench record](../../bench/2026-11-01-ezo-ph-do.md))
- below `2.14`: the output buffer could overflow when the `Cal` and `Cal,0` points were too close together
- below `2.11`: temperature compensation could default to `0` at startup
- below `1.96`: losing power 900 ms into startup could erase the EEPROM

## Timing Notes

The generic repo read hint is conservative for normal DO reads. Calibration and compensation changes should still be treated as command-specific operations rather than inferred from the measurement timing alone.

## Code Implications

The current typed DO module now owns:

- typed parsing for configured outputs
- explicit compensation commands
- calibration query/set/clear helpers

Application code should still treat DO payload shape as configuration-dependent unless output configuration is controlled explicitly.

## Repo Entry Points

- Linux staged examples: `examples/linux/i2c/advanced/do_calibration.c` and `examples/linux/uart/advanced/do_calibration.c`
- Arduino staged example: `examples/arduino/i2c/advanced/do_calibration/do_calibration.ino`
- Compensation workflows: `examples/linux/i2c/advanced/do_workflow.c`, `examples/linux/uart/advanced/do_workflow.c`, `examples/linux/i2c/advanced/do_full_compensation_chain.c`, `examples/linux/uart/advanced/do_full_compensation_chain.c`, and the matching Arduino I2C examples
- Calibration transfer helpers: `src/ezo_calibration_transfer.h`
