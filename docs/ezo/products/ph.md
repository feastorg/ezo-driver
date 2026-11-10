# EZO-pH

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO pH family.

## Product Summary

- measures pH as a single scalar reading
- default transport mode is UART
- default I2C address is `99` (`0x63`)
- normal reads are sub-second, with vendor materials placing them roughly in the `800-900 ms` range
- supports temperature compensation
- supports one-point, two-point, and three-point calibration

## Measurement Model

The pH circuit is one of the simpler EZO products from a parsing perspective:

- normal reads return one pH value
- temperature-compensated reads still return one pH value
- there is no multi-field CSV payload model to manage

That aligns well with the repo's existing `read` and `read with temperature compensation` command kinds.

## Shared Command Families Present

The pH family carries the common families you would expect:

- acquisition
- calibration
- calibration export and import
- identity and status
- LED and find
- sleep and factory reset
- protocol control and mode switching

## Product-Specific Features

The pH family adds three behaviors that matter to the typed helper layer:

### Temperature Compensation

Temperature is part of the measurement model, not just a side setting. The typed product helper treats compensation input as a first-class operation.

### Slope Reporting

The device exposes probe-slope information as a diagnostic output. This is useful for calibration inspection and probe-health workflows, and it is distinct from the ordinary measurement path.

### Extended Range

The product can switch between the standard pH range and an extended range. That toggle affects caller expectations and should not be silently hidden in a generic helper.

## Calibration Procedure

### Calibration Model

pH is the most staged calibration flow in the current product set:

- one point = midpoint only
- two point = midpoint plus low point
- three point = midpoint plus low point plus high point

The command surface is:

- `Cal,mid,n`
- `Cal,low,n`
- `Cal,high,n`
- `Cal,clear`
- `Cal,?`

`Cal,?` reports:

- `?Cal,0` = no calibration
- `?Cal,1` = midpoint only
- `?Cal,2` = midpoint and low point
- `?Cal,3` = midpoint, low point, and high point

Plan on about `900 ms` for `Cal,mid`, `Cal,low`, and `Cal,high`, and about `300 ms` for `Cal,clear` and `Cal,?`.

### Before You Begin

- Start with clean, uncontaminated buffer solutions.
- Prefer simple buffer values such as `7.00`, `4.00`, and `10.00`.
- Always use the midpoint calibration first.
- Never issue `Cal,mid` on an already fully calibrated probe unless you intend to rebuild the whole calibration from the midpoint forward.
- Watch live readings and wait for stabilization before issuing each calibration command.
- Do not leave the probe settling for an excessively long time before calibration; vendor guidance is that very long pre-calibration settling can make post-calibration readings slow to settle too.
- Recalibrate only when there is a real reason. Frequent "just in case" recalibration is more likely to make the probe worse than better.

### Recommended First-Time Order

Use this order for a full first-time calibration:

1. pH 7.00 midpoint
2. pH 4.00 low point
3. pH 10.00 high point

### Step-By-Step Procedure

1. Rinse the probe, place it in the pH 7.00 buffer, and wait until the reading has clearly stabilized. Vendor guidance treats roughly one to two minutes as normal.
2. Issue `Cal,mid,7.00`.
3. Confirm the device acknowledges the command and that subsequent reads settle around the midpoint value.
4. Rinse the probe, place it in the pH 4.00 buffer, wait for stabilization, and issue `Cal,low,4.00`.
5. Confirm the low-point reading now settles around the low buffer.
6. Rinse the probe again, place it in the pH 10.00 buffer, wait for stabilization, and issue `Cal,high,10.00`.
7. Confirm the high-point reading now settles around the high buffer.

If you only need a narrower range, you can stop earlier:

- midpoint only for a one-point calibration
- midpoint plus low for a two-point calibration

### What "Stabilized" Means

Do not calibrate on the first number that looks close. Stabilized means the reading has stopped materially trending and is only making small normal movements between samples. A blind calibration, where the command is sent while the values are still moving, produces drifting post-calibration readings.

### Verification After Calibration

Use `Slope,?` after the full calibration is complete. The slope report is the main health check for a pH probe and calibration quality.

Interpret it at a high level like this:

- the uncalibrated default is `100,100,0`, which is mathematically perfect and therefore suspicious in the real world
- the neutral offset should be close to `0 mV`; a new probe is expected to show a low offset, about `15 mV` or lower
- the acid and base slopes are percentages; a new probe should be above `90%`, and the two values should be similar to each other

Do not over-interpret a poor slope result as automatic probe failure. Contaminated or incorrect buffer solution can produce a bad slope on an otherwise good probe.

### Recovery And Rework

- Use `Cal,clear` if you need to wipe calibration and start over.
- If you rerun `Cal,mid` after a two-point or three-point calibration, treat that as a full reset of the other pH points and redo the full sequence in order.

### Repo Entry Points

- Linux staged examples: `examples/linux/i2c/advanced/ph_calibration.c` and `examples/linux/uart/advanced/ph_calibration.c`
- Arduino staged example: `examples/arduino/i2c/advanced/ph_calibration/ph_calibration.ino`
- Calibration transfer helpers: `src/ezo_calibration_transfer.h`

## Timing Notes

The vendor material for pH reads is slightly context-dependent, but it stays below the generic `1000 ms` fallback in [`src/ezo.c`](../../src/ezo.c). The current repo hint remains acceptable as a conservative default, but product-aware code should prefer the pH-specific expectation.

Calibration and protocol-control operations should still be treated as command-specific rather than inferred from the one-shot read time.

## Code Implications

If the repo grows typed product helpers, pH is a good early candidate because:

- the payload is scalar
- the compensation path maps cleanly to existing helpers
- the product-specific surface is small but meaningful

The current typed pH module now covers temperature compensation, calibration commands/status, slope query, and the extended-range toggle over both I2C and UART.
