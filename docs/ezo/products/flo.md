# EZO-FLO

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO flow meter totalizer family.

## Repo Status

This family is at the `METADATA` tier: the driver identifies it from `i` and records its default address and timing. There are no typed reads or product helpers; use the raw transport APIs. The firmware lookup in `src/ezo_product.h` does not cover it.

## Product Summary

- default transport mode is UART
- default I2C address is `104` (`0x68`)
- the `i` reply identifies it as `FLO`
- `Set` selects an Atlas flow meter size; custom meters use `K`, `Vp` and `P`
- has no calibration command, so the record gives no calibration timing

## Measurement Model

`O` selects total volume and flow rate. The quick reference lists both as enabled by default while the default state and the `R` example show total volume alone, so the record sets the output schema to query-required.

## Timing Notes

The I2C pages print `300 ms` for `R` and for the other commands.
