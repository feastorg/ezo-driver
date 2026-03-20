# Command Taxonomy

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

The EZO product line has heavy command overlap, but the overlap is not uniform enough to justify a single product-agnostic command surface above the current transport APIs.

This taxonomy gives the repo a clean vocabulary for future product-aware helpers.

## Shared Families

These families recur across most or all of the current EZO products covered by this repo.

### Acquisition

- one-shot read
- continuous read enable, disable, and query
- read variants that include compensation input where the product supports them

For repo purposes, acquisition commands split products into two groups:

- scalar products that return one value per read
- configurable products that may return CSV payloads

### Calibration

- calibration query and clear
- one-point, two-point, or three-point flows depending on product
- export and import of calibration state on products that support it

Calibration belongs in a shared family, but the semantics are product-specific enough that a future typed layer should not hide them behind one generic helper.

### Identity And Metadata

- device name
- device information / identification
- device status query

These commands are good candidates for future transport-neutral helpers because they do not materially change by product.

### Device Control

- LED control
- find / visual identification
- sleep
- factory reset

These are widely shared, but some of them trigger reboots or non-standard response sequences, so the command family is common even when the operational details are not identical.

### Protocol Control

- baud-rate control on UART-side flows
- I2C address change on I2C-side flows
- protocol lock
- switch from UART to I2C
- switch from I2C back to UART

This family is documented once in [mode-switching.md](./mode-switching.md) because it is closer to transport state than to sensor measurement behavior.

## Product-Only Families

The products diverge in exactly the places that would shape future typed APIs.

### pH

- temperature compensation
- slope reporting for probe health and calibration inspection
- extended pH range toggle

### ORP

- extended ORP range toggle

### EC

- probe `K` configuration
- temperature compensation
- TDS conversion-factor control
- output enable and disable controls for EC, TDS, salinity, and specific gravity

### DO

- temperature compensation
- salinity compensation
- atmospheric-pressure compensation
- output enable and disable controls for mg/L and percent saturation

### RTD

- temperature scale selection
- onboard logger enable and interval control
- memory recall and clear

### HUM

- output enable and disable controls for humidity, dew point, and temperature
- onboard temperature calibration

## Structural Consequences For The Repo

This taxonomy supports the current repo split:

- `src/ezo_i2c.*` and `src/ezo_uart.*` stay transport-first
- product pages in `docs/ezo/products/` describe the second axis

If the repo later grows product-aware helpers, a reasonable next layer is:

1. keep identity and transport control mostly shared
2. keep acquisition, compensation, and calibration helpers product-specific
3. treat multi-output products as explicit parsers rather than as `parse one double` wrappers
