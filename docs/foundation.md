# Foundation

## Purpose

This document records the repository-level decisions established in Phase 00 so later phases do not invent structure ad hoc.

## Normative Documents

These documents define the project baseline for implementation work:

- [decision-log.md](/mnt/c/Users/Cameron/repos_feast/ezo-i2c-driver/docs/decision-log.md)
- [vision.md](/mnt/c/Users/Cameron/repos_feast/ezo-i2c-driver/docs/vision.md)
- [architecture.md](/mnt/c/Users/Cameron/repos_feast/ezo-i2c-driver/docs/architecture.md)
- [master-plan.md](/mnt/c/Users/Cameron/repos_feast/ezo-i2c-driver/docs/master-plan.md)
- [planning/README.md](/mnt/c/Users/Cameron/repos_feast/ezo-i2c-driver/planning/README.md)
- all `planning/phase-*.md` documents for execution sequencing

If a future implementation decision conflicts with these documents, the decision must be made explicit in the decision log rather than being introduced implicitly in code.

## Repository Layout

```text
include/           Public C and C++ headers
src/               Core C implementation
adapters/
  arduino/         Arduino-specific transport integration
  linux/           Linux-specific transport integration
tests/
  fakes/           Fake transports and test doubles
examples/          Focused usage examples only
docs/              Product, architecture, and planning documents
planning/          Detailed execution phases and audits
_reference/        Legacy reference material, read-only by policy
```

## Ownership Boundaries

### Core

- `include/` and `src/` are the source of truth for the product implementation.
- The core must remain platform-agnostic.

### Adapters

- `adapters/arduino/` may depend on Arduino types such as `TwoWire`.
- `adapters/linux/` may depend on Linux I2C facilities.
- Adapter code must not redefine protocol semantics.

### Tests

- `tests/fakes/` holds fake transports and supporting test doubles.
- Core behavior should be validated here before relying on hardware.

### Examples

- `examples/` is for narrow, validation-oriented examples.
- Examples are not a substitute for API definition.

### Reference

- `_reference/` is protocol and behavior reference material only.
- New product code must not be added under `_reference/`.
- `_reference/` should not be treated as partially live implementation code.

## Initial Build Skeleton

The top-level [CMakeLists.txt](/mnt/c/Users/Cameron/repos_feast/ezo-i2c-driver/CMakeLists.txt) is intentionally minimal in Phase 00.

Its purpose is to:

- establish CMake as the primary build entry point
- define the major optional build areas
- avoid inventing targets before the API and contract phase completes

Planned target groups:

- core library
- test targets
- Arduino adapter targets
- Linux adapter targets
- C++ wrapper integration targets
- example targets

## Phase 00 Exit State

Phase 00 is considered complete when:

- the repository structure exists
- directory ownership is explicit
- `planning/` is versioned and no longer ignored
- `_reference/` is explicitly fenced off as reference-only
- a minimal CMake entry point exists for later phases
