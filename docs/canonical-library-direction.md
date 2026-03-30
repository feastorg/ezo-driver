# Canonical Library Direction

## Purpose

This document records the stable long-term direction for `ezo-driver`.

It is not a phase checklist. It exists so the repo's intended end state is tracked in `docs/` instead of living only in disposable scratch notes.

## Current baseline

The current baseline is transport-complete for the core library shape:

- shared public `ezo.h` surface
- shared control-plane layer in `src/ezo_control.*`
- shared calibration-transfer layer in `src/ezo_calibration_transfer.*`
- formal product identity and metadata layer in `src/ezo_product.*`
- shared parser, sequence, and schema infrastructure in `src/ezo_parse.*` and `src/ezo_schema.*`
- typed product modules in `src/ezo_ph.*`, `src/ezo_orp.*`, `src/ezo_rtd.*`, `src/ezo_ec.*`, `src/ezo_do.*`, and `src/ezo_hum.*`
- explicit I2C and UART C driver families
- thin I2C C++ wrapper
- Arduino adapters for I2C and UART
- Linux I2C adapter and Linux host POSIX UART adapter
- curated EZO product and protocol docs under `docs/ezo/`

That baseline is stable enough to treat the transport and initial product architecture as settled.

## Core conclusion

The canonical library must stay organized around three separate axes:

1. transport
2. product
3. platform

The library should be universal at the repo level, not by pretending every device and transport is the same thing.

## Principles to keep

1. The C API remains the source of truth.
2. The core stays synchronous.
3. The core does not allocate dynamically.
4. The core does not sleep internally.
5. Callers own timing and buffers.
6. I2C and UART remain explicit sibling driver families.
7. Platform adapters stay thin.
8. Protocol state such as continuous mode or UART response-code mode must not be hidden or guessed.
9. Product behavior should sit above transport framing, not inside it.
10. Repo docs stay original and summary-oriented rather than mirroring vendor manuals.

## Planned architecture

The durable library stack should continue to look like this:

1. shared public base layer
2. shared internal utility layer
3. transport cores
4. product identity and metadata layer
5. product helper modules
6. thin C++ convenience wrappers where justified
7. platform adapters

## What the product layer should provide

The canonical library now has a formal product model rather than freeform command strings alone.

That layer should eventually cover:

- product ID and family enums
- default I2C addresses and default mode expectations
- capability flags
- product-aware timing and output-shape metadata
- device-info parsing and identification helpers
- one coherent support matrix for officially supported EZO products

Start with a static hand-authored registry. Do not add code generation unless the maintenance burden becomes real.

## Product module strategy

Product logic should be shared by product family, then connected to transport-specific execution helpers.

That means:

- transport cores own framing and transport errors
- product modules own command vocabulary and payload interpretation
- thin product-by-transport helpers connect the two

This avoids both duplicated product logic and fake universal transport abstractions.

## Rollout order

The initial rollout order is now established in the shipped surface:

- scalar-output products first: pH, ORP, RTD
- configurable multi-output products second: EC, DO, HUM

## Supporting work still needed

The main intentionally deferred surface is:

- a matching UART C++ wrapper only if the C surface justifies it

The tracked support matrix and curated public API layer docs now live in:

- `docs/support-matrix.md`
- `docs/public-api-layers.md`
- `docs/product-onboarding.md`

## Explicit non-goals

The canonical direction is not:

- one fake universal `ezo_device_t`
- one transport abstraction that erases I2C and UART differences
- a vendor-manual clone inside the repo
- a grab bag of loosely related helpers with no product model

## How to use this document

Treat this as the fixed long-term direction for future product work.

Use `docs/product-onboarding.md` for the repeatable growth checklist, and update this document only when the long-term architectural intent changes.
