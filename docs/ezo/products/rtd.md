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

That means a future typed helper should either:

- preserve unit state explicitly
- or normalize units in a clearly documented way

## Shared Command Families Present

The RTD family includes the common acquisition, calibration, identity, control, and protocol-switching families, plus calibration export and import.

## Product-Specific Features

### Temperature Scale

Scale selection changes the meaning of ordinary read results even though the payload remains scalar text.

### Data Logger And Memory

RTD is the outlier in the current product set because it includes onboard logging and stored-reading recall. Those commands do not belong in a generic EZO abstraction.

## Timing Notes

The generic repo read hint is conservative for RTD and broadly safe across the vendor timing views. Logger and memory operations should still be treated as their own product behaviors rather than folded into the normal read path.

## Code Implications

A future RTD helper layer should probably group:

- read and calibration helpers
- unit selection
- logger control
- memory recall and clear

RTD is still simpler than EC, DO, and HUM from a parsing perspective, but broader from a device-feature perspective.
