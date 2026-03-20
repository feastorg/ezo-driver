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

The pH family adds three behaviors that matter to future typed helpers:

### Temperature Compensation

Temperature is part of the measurement model, not just a side setting. A future product helper should treat compensation input as a first-class operation.

### Slope Reporting

The device exposes probe-slope information as a diagnostic output. This is useful for calibration inspection and probe-health workflows, and it is distinct from the ordinary measurement path.

### Extended Range

The product can switch between the standard pH range and an extended range. That toggle affects caller expectations and should not be silently hidden in a generic helper.

## Timing Notes

The vendor material for pH reads is slightly context-dependent, but it stays below the generic `1000 ms` fallback in [`src/ezo.c`](../../src/ezo.c). The current repo hint remains acceptable as a conservative default, but product-aware code should prefer the pH-specific expectation.

Calibration and protocol-control operations should still be treated as command-specific rather than inferred from the one-shot read time.

## Code Implications

If the repo grows typed product helpers, pH is a good early candidate because:

- the payload is scalar
- the compensation path maps cleanly to existing helpers
- the product-specific surface is small but meaningful

The likely next layer is a small pH helper set for temperature compensation, slope query, extended range, and calibration-state inspection.
