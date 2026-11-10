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

The documented output-config query reply uses the alternate vendor form `?,O,...` rather than the more common `?Prefix,...` query shape. The typed HUM parser accepts that vendor response directly.

### Temperature Calibration

The product is factory calibrated, but the onboard temperature-calibration path still matters because humidity interpretation is temperature-sensitive. The current helper treats temperature calibration as a product-specific operation, not as a generic calibration abstraction.

## Calibration Procedure

### Calibration Model

HUM is different from the other supported products:

- the humidity sensor is factory calibrated
- the user-facing calibration path is `Tcal`, which adjusts the onboard temperature sensor rather than running a classical multi-point humidity calibration

The command surface is:

- `Tcal,t`
- `Tcal,clear`
- `Tcal,?`

`Tcal,?` reports:

- `?Tcal,0` = no custom temperature calibration
- `?Tcal,1` = custom temperature calibration present

The documented I2C processing delay for this family is `300 ms`.

### Before You Begin

- Treat `Tcal` as a correction for the onboard temperature sensor, not as a general humidity recalibration command.
- Use a trusted temperature reference near the sensor.
- Let the HUM device and the reference reach the same thermal environment before you calibrate.
- Do not use `Tcal` casually. Because humidity calculation depends strongly on temperature, the right time to use it is when you have evidence that the onboard temperature reading is consistently offset from a better reference.

### Step-By-Step Procedure

1. Place the HUM device and a trusted temperature reference together in the same stable environment.
2. Wait until both readings have stabilized at the same ambient condition.
3. Read the trusted reference temperature.
4. Issue `Tcal,<reference temperature>` using that known value.
5. Confirm the command succeeds and that later temperature reads line up with the reference more closely.

### Clear And Query

- Use `Tcal,?` to check whether a custom temperature calibration is present.
- Use `Tcal,clear` if you need to remove the custom correction and return to the factory baseline.

### Scope Warning

This calibration path exists because humidity is temperature-sensitive. It does not mean the device expects routine user humidity calibration in the same way pH, ORP, EC, DO, or RTD do.

## Firmware Caveats

Read the circuit's firmware with `i` before relying on a feature that arrived late. These versions come from the vendor's firmware changelog; it has not been checked on a circuit.

The temperature calibration helpers (`Tcal`) need firmware `1.1`. The EZO-HUM-C changelog lists it as `1.1` and the embedded-probe changelog as `1.01`, both dated March 2024, so check what your circuit reports.

## Timing Notes

HUM is the strongest example of a transport-specific timing difference in the current product set:

- UART behavior is roughly one-second cadence
- I2C behavior can be substantially faster

That reinforces the rule that repo timing hints are fallback defaults, not a product-accurate timing table.

## Code Implications

The current typed HUM module now owns:

- typed parsing for enabled outputs
- output-enable configuration
- temperature-calibration helpers

It should also keep application writers aware that enclosure heat and board temperature can materially affect readings, even though those environment concerns sit outside the transport layer itself.

## Repo Entry Points

- Linux staged examples: `examples/linux/i2c/advanced/hum_temperature_calibration.c` and `examples/linux/uart/advanced/hum_temperature_calibration.c`
- Arduino staged example: `examples/arduino/i2c/advanced/hum_temperature_calibration/hum_temperature_calibration.ino`
