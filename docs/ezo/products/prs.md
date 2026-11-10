# EZO-PRS

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO pressure family.

## Repo Status

This family is at the `METADATA` tier: the driver identifies it from `i` and records its default address and timing. There are no typed reads or product helpers; use the raw transport APIs. The firmware lookup in `src/ezo_product.h` does not cover it.

## Product Summary

- default transport mode is UART
- default I2C address is `106` (`0x6A`)
- the `i` reply identifies it as `PRS`, printed with a space before the version on the I2C page
- `U` selects psi, atm, bar, kPa, inches of water or cm of water, and psi is the default
- firmware `1.03` added vacuum sensing

## Measurement Model

A reading is one pressure value in the selected unit. `U,1` adds a unit identifier to the output, so a parser cannot assume a bare number once that is on.

## Timing Notes

The I2C pages print `900 ms` for `R`, `Cal,n` and `Cal,0`, and `300 ms` for most other commands.
