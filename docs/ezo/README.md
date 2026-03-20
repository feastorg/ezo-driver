# EZO Documentation

> Notice: This subtree is original repo-authored documentation informed by Atlas Scientific product documentation. It is not an official Atlas Scientific manual and intentionally omits copied tables, hardware walkthroughs, and exhaustive command listings.

## Purpose

`docs/ezo/` is the product and protocol context layer for this repo.

It exists to do three things:

1. summarize shared EZO transport behavior once
2. summarize each supported EZO product family once
3. keep repo-facing guidance separate from vendor-manual reproduction

This structure follows the codebase. The current library is split first by transport family (`I2C` and `UART`), while product behavior is a second axis that matters for future helpers, examples, tests, and parsing.

## How To Use This Tree

Start here if you are deciding where a fact belongs:

- [source-policy.md](./source-policy.md): provenance, copyright, and summary rules
- [common/command-taxonomy.md](./common/command-taxonomy.md): shared command families and product-only families
- [common/uart.md](./common/uart.md): UART framing and response behavior
- [common/i2c.md](./common/i2c.md): I2C transaction and status-byte behavior
- [common/mode-switching.md](./common/mode-switching.md): UART/I2C switching, protocol lock, and reboot expectations

Then use the relevant product page:

- [products/ph.md](./products/ph.md)
- [products/orp.md](./products/orp.md)
- [products/ec.md](./products/ec.md)
- [products/do.md](./products/do.md)
- [products/rtd.md](./products/rtd.md)
- [products/hum.md](./products/hum.md)

## Structure Decision

This repo does not organize docs as `ph-uart.md`, `ph-i2c.md`, `ec-uart.md`, and so on.

That layout would duplicate the same transport rules across every product and would pull the repo toward vendor-manual mirroring. Instead:

- common protocol mechanics live in `common/`
- product behavior lives in `products/`

If a product page grows too large later, split it by concern such as calibration or measurement model, not by transport.

## What These Docs Are Not

These docs are not a replacement for official vendor documentation.

Use them for:

- repo architecture decisions
- product-aware API planning
- example and test design
- caller expectations around output shape and timing

Use the official Atlas Scientific materials for:

- full command syntax
- wiring and soldering instructions
- hardware recovery procedures
- exhaustive operational workflows
