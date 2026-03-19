# Architecture

## Overview

The library is organized as a layered system:

1. a platform-agnostic C99 core
2. optional platform adapters
3. a thin C++11 wrapper
4. examples and utilities outside the core API

This keeps protocol logic centralized while allowing multiple platforms and language surfaces.

The detailed public contract for the current implementation stage lives in [api-contract.md](./api-contract.md).

## Architectural Goals

- keep the core independent of platform SDKs
- expose the same underlying behavior to C and C++
- make transport behavior injectable and testable
- isolate packaging and example concerns from protocol logic

## Layer Model

### 1. Core C library

Responsibility:

- command construction
- response decoding
- device state management
- protocol-level validation
- explicit timing metadata
- separation of library errors from device status codes

Must not depend on:

- Arduino headers
- Linux I2C headers
- dynamic allocation
- C++ runtime or STL

Expected locations:

- `include/`
- `src/`

### 2. Transport interface

Responsibility:

- define the contract between the core and the actual I2C implementation

Expected model:

- function pointers/callbacks
- opaque caller-owned context
- explicit read/write operations

Likely concerns carried by the interface:

- bus transaction execution
- optional platform wait helper hooks outside the core

### 3. Platform adapters

Responsibility:

- bridge platform-specific I2C APIs into the transport contract

Initial adapters:

- `adapters/arduino/`
- `adapters/linux/`

Arduino adapter:

- wraps `TwoWire`
- stays outside the core public C headers

Linux adapter:

- wraps the Linux I2C mechanism selected for implementation
- exposes the same transport behavior expected by the core

### 4. C++ wrapper

Responsibility:

- provide ergonomic C++ access to the C core
- preserve feature parity with the C surface
- avoid reimplementing protocol logic

Constraints:

- public baseline is C++11
- wrapper should remain thin
- wrapper is header-only

### 5. Examples and utilities

Responsibility:

- demonstrate correct use of the core and adapters
- provide reference integration patterns

Not part of the core API:

- printing helpers
- command shells
- sequencer helpers

These may exist later as separate supporting modules, but they do not define the architecture.

## Public API Direction

The first public API should be generic rather than device-specific.

Core operations should cover:

- initialize or bind a device instance
- send an arbitrary command
- read and decode a device response
- inspect library-level error results
- inspect device response/status
- expose required wait times for command classes

Deferred from the first API:

- typed device helpers
- async API
- hidden internal timing behavior

## State and Data Ownership

### Device state

The device object should store only what is necessary for correct protocol interaction, such as:

- device address
- transport reference
- last known device status
- any minimal command-context metadata required by the API

### Buffer ownership

- callers own buffers
- APIs accept buffer pointer plus length
- truncation and invalid-size behavior must be explicit

### Memory policy

- no heap allocation in the core
- no hidden global mutable state in the core

## Error Model

The architecture separates:

1. library errors
2. transport failures
3. device-reported status codes
4. parsed payload results

This avoids conflating transport failure with a valid device response that indicates `not ready`, `no data`, or command failure.

## Timing Model

The core does not sleep.

Instead, the architecture should support:

- returning or exposing required wait durations
- caller-controlled blocking behavior
- optional wait helpers outside the core for convenience

This preserves portability and lets applications choose blocking, cooperative scheduling, or custom timing strategies.

## Testing Strategy

Primary testing target:

- the C core against a fake transport

Test categories:

- command formatting
- response parsing
- status/error mapping
- buffer boundary behavior
- timing metadata behavior

Secondary validation:

- adapter smoke tests
- example builds

## Packaging and Build

Primary development flow:

- CMake

Additional distribution surfaces:

- Arduino library metadata
- planned PlatformIO support

The build layout should support:

- host-side tests
- Linux adapter builds
- wrapper builds
- example organization without distorting the core structure

## Proposed Repository Layout

```text
include/
src/
adapters/
  arduino/
  linux/
tests/
examples/
docs/
_reference/
```

## Architectural Risks

1. Over-designing the transport boundary
   Keep it small and centered on the transactions the protocol actually needs.

2. Letting adapter concerns leak into the core
   Platform code must remain outside the core.

3. Reintroducing legacy utility code into the public API
   Keep demos and helpers separate from protocol surface area.

4. Under-specifying timing and result semantics
   The API documentation must define these precisely before implementation starts.
