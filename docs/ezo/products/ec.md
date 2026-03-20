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

When more than one field is enabled, vendor documentation defines a fixed output order. That means any future EC helper should parse an explicit CSV schema rather than call `ezo_parse_double()` on the whole payload.

## Shared Command Families Present

The EC family includes the common acquisition, calibration, identity, control, and protocol-switching families, plus calibration export and import.

## Product-Specific Features

### Probe Configuration

The EC product exposes probe `K` configuration. That configuration changes how the device should be interpreted in an application and belongs in any future typed helper layer.

### Temperature Compensation

Temperature input directly affects readings and should be modeled as part of the product workflow, not as an afterthought.

### TDS Conversion Factor

The device can change its TDS conversion factor. That is not a transport concern; it is product behavior with application-level impact.

### Output Selection

The device can enable or disable its output fields independently. This changes the payload shape and is the strongest reason not to treat EC as a generic "read one value" device.

## Timing Notes

The current repo read fallback of `1000 ms` is conservative for EC reads. Product-aware callers can usually wait less than that, but exact timing should still be treated as command-specific for calibration and control operations.

## Code Implications

Future EC helpers should likely own:

- a typed output parser
- output-enable configuration
- probe `K` configuration
- temperature-compensation helpers
- calibration helpers

That layer should sit above the current transport APIs rather than complicate `src/ezo_i2c.*` or `src/ezo_uart.*`.
