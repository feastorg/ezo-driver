# Migration Guide

## Purpose

This guide shows how to move from raw transport usage to the higher-level public surface without losing the repo's explicit workflow model.

## Path 1: Raw Transport To Identity-Aware Code

Start from:

- `ezo_send_command(..., "i", ...)`
- `ezo_read_response(...)`
- hand-parsed `?i,...`

Move to:

- `ezo_control_send_info_query_*()`
- `ezo_control_read_info_*()`
- `ezo_product_get_metadata()`

Use this path when your application first discovers what product is attached and then chooses a product-specific flow.

## Path 2: Raw Reads To Typed Product Reads

Start from:

- `ezo_send_read()` or `ezo_uart_send_read()`
- raw text parsing

Move to:

- `ezo_ph_*`, `ezo_orp_*`, `ezo_rtd_*`
- `ezo_ec_*`, `ezo_do_*`, `ezo_hum_*`

The typed modules preserve the repo's explicit flow:

1. send the command
2. wait using the returned timing hint
3. read the typed response

The library still does not sleep internally.

## Path 3: Raw Shared Commands To Shared Control Helpers

Start from raw strings such as:

- `Name,?`
- `Status`
- `L,?`
- `*OK,?`
- `Factory`

Move to:

- `ezo_control_send_*_*()`
- `ezo_control_read_*_*()`

Use this path when the command is common across products and you want a stable typed parse result instead of open-coded string handling.

## Path 4: Raw Export/Import Loops To Calibration Transfer Helpers

Start from:

- raw `Export,?`, `Export`, and `Import,...`
- handwritten parsing of chunk counts and terminal tokens

Move to:

- `ezo_calibration_send_*_*()`
- `ezo_calibration_read_*_*()`

These helpers still leave loop ownership with the caller, which is deliberate. Reboot and reconnect behavior remains explicit application logic.

## Path 5: Product Discovery To Canonical App Flow

For a new application, the normal progression is:

1. bring up transport with `ezo_i2c_*` or `ezo_uart_*`
2. identify the product with `ezo_control` and `ezo_product`
3. switch to the typed product module for ongoing measurement/configuration work
4. use shared control helpers only for the common admin/protocol features

## What Does Not Change

Higher layers do not change these repo rules:

- transports remain explicit
- callers own timing
- callers own buffers
- callers own reboot/reconnect handling
- multi-line UART workflows are still explicit
