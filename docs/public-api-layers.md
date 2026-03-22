# Public API Layers

## Purpose

This document tells a new user where to start.

The library now has enough surface area that "use the repo" is no longer specific enough.

## Layer Order

Use the highest layer that matches your actual need.

### 1. Raw Transport APIs

Headers:

- `src/ezo_i2c.h`
- `src/ezo_uart.h`

Use this layer when you:

- need vendor-command parity
- want to prototype with raw command strings
- need direct access to I2C status bytes or UART response kinds
- are building your own product-specific workflow on top

This is the right entry point for uncommon commands, debugging, and transport bring-up.

### 2. Product Identity And Metadata

Header:

- `src/ezo_product.h`

Use this layer when you:

- want to identify a device from `i` output
- need default addresses, timing, support tiers, or capability flags
- want to branch logic by product family without hardcoding string parsing

This layer is facts and lookups only. It does not send commands for you.

### 3. Shared Control And Transfer Helpers

Headers:

- `src/ezo_control.h`
- `src/ezo_calibration_transfer.h`

Use this layer when you need common workflows such as:

- info, name, and status queries
- LED, sleep, factory reset, and find
- UART response-code mode, baud, protocol lock, and mode switching
- calibration export and import primitives

These helpers stay transport-explicit and do not hide reconnect or reboot handling.

### 4. Typed Product Helpers

Headers:

- scalar products: `src/ezo_ph.h`, `src/ezo_orp.h`, `src/ezo_rtd.h`
- multi-output products: `src/ezo_ec.h`, `src/ezo_do.h`, `src/ezo_hum.h`

Use this layer when you want:

- typed reads instead of raw text parsing
- product-aware configuration helpers
- calibration and advanced product features behind explicit typed APIs

This is the normal entry point for application code that already knows which EZO product it is talking to.

### 5. I2C C++ Convenience Layer

Header:

- `src/ezo_i2c.hpp`

This wrapper remains intentionally thin and only exists for the I2C C surface.

## Start Here By Use Case

- Hardware bring-up: start with raw `ezo_i2c_*` or `ezo_uart_*`
- Identify an unknown device: use `ezo_control` info helpers plus `ezo_product`
- Read measurements from a known product: use the typed product module directly
- Change shared device settings: use `ezo_control`
- Transfer calibration blobs: use `ezo_calibration_transfer`

## UART C++ Decision

The UART C++ wrapper is deferred again.

Reasoning:

- the C UART surface is already explicit and readable
- there is no matching user need that justifies another maintained wrapper layer yet
- Phase 7 is about making the current surface legible, not duplicating it

If that changes later, the wrapper should stay as thin as `src/ezo_i2c.hpp` and add no independent protocol logic.
