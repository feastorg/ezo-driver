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

That matters because it changes the valid measurement envelope without changing the transport framing. A future typed helper should model range state explicitly if it exposes this feature at all.

## Timing Notes

The broad repo read hint of `1000 ms` matches the product summary well enough for ORP. That makes ORP a good example of where the generic timing hint is conservative but not misleading.

## Code Implications

If a typed ORP helper is added later, it can stay small:

- plain read
- calibration query and control
- extended-range query and control

ORP does not need a multi-value parser layer, which keeps it separate from EC, DO, and HUM in future API design.
