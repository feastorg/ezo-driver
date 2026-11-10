# EZO-CO2

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page captures the repo-relevant behavior of the EZO carbon dioxide family.

## Repo Status

This family is at the `TYPED_READ` tier. `src/ezo_co2.h` covers `R` and `O` over I2C and UART. Calibration and `Alarm` are deferred; use the raw transport APIs for those. The firmware lookup in `src/ezo_product.h` does not cover it. None of this has been run on a circuit.

## Product Summary

- reads gaseous CO2 from `0` to `10,000 ppm`
- default transport mode is UART
- default I2C address is `105` (`0x69`)
- the `i` reply identifies it as `CO2`
- ships factory calibrated; the datasheet advises against custom calibration without scientific-grade gases
- no temperature compensation command; the datasheet gives a `10 s` warm-up

## Measurement Model

A reading is one CO2 value in ppm. `O,t,1` adds the internal temperature to the output string, and that is off by default, so the default output is one field. No datasheet page prints a two-value reading; the driver parses it as ppm, then temperature, inferred from the `?O,ppm,t` query reply. Confirm that on a circuit before relying on it.

For the 10 s after power-up or wake the circuit outputs `*warm`. The read helpers return success with `warming` set and no outputs present. Whether `*OK` follows `*warm` over UART is not printed; the UART reader expects it to, as for any reading.

## Timing Notes

The I2C pages print `900 ms` for `R` and for `Cal,n` and `Cal,0`, and `300 ms` for most other commands.
