# EZO-HUM

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO humidity family.

## Product Summary

- default transport mode is UART
- default I2C address is `111` (`0x6F`)
- UART reads are roughly one reading per second
- I2C reads can be much faster, around `300 ms`
- reports relative humidity and can also report dew point and air temperature
- ships factory calibrated, with an additional onboard temperature-calibration path

## Measurement Model

HUM is a configurable multi-output device.

Depending on device configuration, a reading may include:

- relative humidity
- dew point
- air temperature

When more than one output is enabled, the payload becomes CSV-like rather than scalar. That means generic single-value parsing is not sufficient for a full HUM helper surface.

## Shared Command Families Present

The HUM family includes acquisition, identity, control, and protocol-switching families. It differs from most of the other products because its calibration surface is narrower and more focused on temperature correction than on classical probe calibration.

## Product-Specific Features

### Output Selection

The device can enable or disable humidity, dew-point, and temperature outputs independently. Payload shape is therefore configuration-dependent.

### Temperature Calibration

The product is factory calibrated, but the onboard temperature-calibration path still matters because humidity interpretation is temperature-sensitive. A future helper should treat temperature calibration as a product-specific operation, not as a generic calibration abstraction.

## Timing Notes

HUM is the strongest example of a transport-specific timing difference in the current product set:

- UART behavior is roughly one-second cadence
- I2C behavior can be substantially faster

That reinforces the rule that repo timing hints are fallback defaults, not a product-accurate timing table.

## Code Implications

A future HUM helper layer should likely own:

- typed parsing for enabled outputs
- output-enable configuration
- temperature-calibration helpers

It should also keep application writers aware that enclosure heat and board temperature can materially affect readings, even though those environment concerns sit outside the transport layer itself.
