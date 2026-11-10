# EZO-PMP and EZO-PMP-L

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO dosing pump families.

## Repo Status

This family is at the `METADATA` tier: the driver identifies it from `i` and records its default address and timing. There are no typed reads or product helpers; use the raw transport APIs. The firmware lookup in `src/ezo_product.h` does not cover it.

## Product Summary

- default transport mode is UART
- default I2C addresses are `103` (`0x67`) for EZO-PMP and `109` (`0x6D`) for EZO-PMP-L
- the `i` replies identify them as `PMP` and `PMPL`
- these are actuators: `D` starts and stops dispensing
- calibration is against a dispensed volume

## Measurement Model

`R` returns the volume dispensed, not a sensor measurement, so the record claims no measurement capability. `O` selects volume, total volume and absolute total volume; the record sets the output schema to query-required. Typed support waits on a design for actuators, which do not fit the read-a-measurement model.

## Timing Notes

The I2C pages print a `300 ms` header and `300 ms` example rows for `R`, while the LED figure on the same page says `600 ms`. The record uses `600 ms`: waiting longer costs little, and a read that comes too soon only returns `NOT_READY`. `Cal` and the other commands print `300 ms`.
