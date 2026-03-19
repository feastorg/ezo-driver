# Vision

## Product Intent

Build a small, dependable, cross-platform EZO I2C driver that can be used in embedded and host environments without changing the core library design.

The project should be:

- simple to understand
- clean to integrate
- complete enough to be practical on day one
- disciplined about portability, testing, and boundaries

## Problem

The reference implementation proves the on-wire protocol, but it is tightly coupled to Arduino types, mixes driver logic with utility/demo code, and does not present a clean cross-platform foundation.

That makes it harder to:

- reuse the driver on Linux
- expose both C and C++ interfaces cleanly
- test the core logic in isolation
- evolve the library without inheriting legacy API constraints

## Vision Statement

The new library will provide one portable protocol core, thin platform adapters, and a minimal public API that behaves consistently across Arduino and Linux.

Users should be able to:

- send commands and read responses from any EZO device
- integrate with their own scheduling model
- choose C or C++ without losing capability
- swap transports without rewriting driver logic

## Product Principles

1. Portability before convenience
   The core must not depend on Arduino- or OS-specific types.

2. One implementation of truth
   Protocol behavior lives in the C core. Everything else layers on top.

3. Explicit behavior
   Timing, buffers, and error handling should be visible in the API rather than hidden behind assumptions.

4. Small public surface
   The core API should stay generic and focused on protocol operations.

5. Good embedded hygiene
   No dynamic allocation in the core. Caller-owned buffers. Predictable behavior.

6. Testability by design
   The transport boundary must make the core easy to test without hardware.

## Non-Goals for the First Milestone

- source compatibility with the legacy Arduino API
- an async/state-machine core API
- device-specific typed helpers in the core
- bringing legacy utility printing or sequencer helpers into the main API

## Success Criteria

The first complete milestone is successful when the project provides:

- a stable C99 core API
- a thin C++11 wrapper
- Arduino and Linux adapters
- deterministic unit tests against a fake transport
- a few focused examples showing correct integration patterns
- packaging that supports both host development and Arduino consumption

## User Outcomes

For embedded users:

- use the library on Arduino without inheriting Arduino-specific design limitations in the core

For Linux users:

- use the same protocol model against a Linux I2C adapter with host-side build and test workflows

For maintainers:

- evolve the codebase from a clear architecture rather than a pile of examples and helpers
