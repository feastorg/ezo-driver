# EZO-RGB

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO color family.

## Repo Status

This family is at the `METADATA` tier: the driver identifies it from `i` and records its default address and timing. There are no typed reads or product helpers; use the raw transport APIs. The firmware lookup in `src/ezo_product.h` does not cover it.

## Product Summary

- default transport mode is UART
- default I2C address is `112` (`0x70`)
- the `i` reply identifies it as `RGB`
- has separate target and indicator LED controls (`L` and `iL`) and a color-matching command (`M`)
- calibrates with a single `Cal` against a white target

## Measurement Model

`O` selects RGB, lux and CIE 1931 outputs, and the default is RGB, which is three values. The record sets the output schema to query-required, so a caller reads `O,?` rather than assuming a field count.

## Timing Notes

The I2C pages print `300 ms` for `R` and `Cal`.
