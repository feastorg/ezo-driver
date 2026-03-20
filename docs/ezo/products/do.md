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

That is a strong argument for a future typed helper layer, because these settings shape both calibration practice and measurement interpretation.

### Output Selection

The product can enable or disable mg/L and percent-saturation outputs. As with EC and HUM, payload parsing belongs in a product-aware layer.

## Timing Notes

The generic repo read hint is conservative for normal DO reads. Calibration and compensation changes should still be treated as command-specific operations rather than inferred from the measurement timing alone.

## Code Implications

A future DO helper layer should probably own:

- typed parsing for configured outputs
- explicit compensation commands
- calibration helpers

Without that layer, application code should treat DO payload shape as configuration-dependent.
