# EZO-CO2

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO carbon dioxide family.

## Repo Status

This family is at the `METADATA` tier: the driver identifies it from `i` and records its default address and timing. There are no typed reads or product helpers; use the raw transport APIs. The firmware lookup in `src/ezo_product.h` does not cover it.

## Product Summary

- reads gaseous CO2 from `0` to `10,000 ppm`
- default transport mode is UART
- default I2C address is `105` (`0x69`)
- the `i` reply identifies it as `CO2`
- ships factory calibrated; the datasheet advises against custom calibration without scientific-grade gases
- no temperature compensation command; the datasheet gives a `10 s` warm-up

## Measurement Model

A reading is one CO2 value in ppm. `O,t,1` adds the internal temperature to the output string, and that is off by default, so the default output is one field.

## Timing Notes

The I2C pages print `900 ms` for `R` and for `Cal,n` and `Cal,0`, and `300 ms` for most other commands.
