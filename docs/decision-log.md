# Engineering Decision Log

Status: Accepted baseline for design and planning

Purpose: record the architectural decisions that govern the rewrite and serve as the input for the vision, architecture, and planning documents.

## ED-001: This is a rewrite, not a compatibility refactor

Date: 2026-03-19
Status: Accepted

Context:

The reference library captures the existing device protocol behavior, but its API is tightly shaped around Arduino and carries avoidable design debt.

Decision:

- The new project will define a new API.
- The old API will not constrain the design.
- The `_reference/` tree is protocol and behavior reference material only.
- Migration guidance may be provided later, but source compatibility is not a goal.

Consequences:

- We can optimize for portability, clarity, and long-term maintainability.
- Existing users of the old API will need a migration path rather than drop-in compatibility.

## ED-002: The core implementation is C99

Date: 2026-03-19
Status: Accepted

Context:

The library must work well in embedded environments, remain portable across platforms, and be consumable from both C and C++.

Decision:

- The source of truth will be a pure C99 core.
- The C core will expose the stable protocol, parsing, and transport-facing APIs.

Consequences:

- The project gets a clean ABI and broad portability.
- Core code must avoid C++ language features and dependencies.

## ED-003: C++ support is a thin wrapper over the C core

Date: 2026-03-19
Status: Accepted

Context:

C++ ergonomics are useful, especially on Arduino, but two independent implementations would create drift and maintenance cost.

Decision:

- C++ support will wrap the C core rather than reimplement it.
- The public wrapper baseline is C++11.
- The wrapper should stay thin, be header-only, and require explicit initialization.

Consequences:

- Most functionality and testing stays concentrated in the C core.
- The wrapper must avoid newer C++ requirements in its public API.

## ED-004: Transport is injected through callbacks

Date: 2026-03-19
Status: Accepted

Context:

The core must not depend on Arduino `TwoWire`, Linux `i2c-dev`, or any other platform-specific transport type.

Decision:

- The core will use a transport interface based on callbacks/function pointers plus caller-owned context.
- Platform integrations will live in adapters.

Consequences:

- The core remains platform-agnostic.
- Adapter design becomes a first-class part of the architecture.

## ED-005: The initial API is synchronous

Date: 2026-03-19
Status: Accepted

Context:

A synchronous API is the smallest complete design for a first release and maps directly onto the reference protocol behavior.

Decision:

- The initial core API will be synchronous.
- Future async/state-machine support is allowed, but not part of the first design.

Consequences:

- The first release is easier to implement and validate.
- Documentation should note async support as a future extension point, not a current feature.

## ED-006: The core owns no dynamic memory

Date: 2026-03-19
Status: Accepted

Context:

The library must behave predictably on embedded targets and stay easy to test.

Decision:

- No dynamic allocation in the core.
- Buffers are caller-owned and always passed with explicit lengths.

Consequences:

- Callers have full control over memory.
- APIs must make buffer sizing and truncation behavior explicit.

## ED-007: Library errors are separate from device status codes

Date: 2026-03-19
Status: Accepted

Context:

The reference code mixes transport, parse, and device status into one narrow status flow, which obscures failure modes.

Decision:

- Library-level errors and transport failures will be distinct from device response/status codes.
- The API should make it easy to inspect both.

Consequences:

- Error handling becomes clearer and more diagnosable.
- Public API design needs explicit result and status structures or equivalent primitives.

## ED-008: Timing is explicit in the core

Date: 2026-03-19
Status: Accepted

Context:

The devices require wait periods between request and response, but hiding sleeps inside the core would make scheduling and integration less predictable.

Decision:

- The core will not sleep internally.
- The core will expose timing requirements explicitly.
- Optional wait helpers may exist outside the core, such as in adapters or utilities.

Consequences:

- Callers keep control over blocking behavior and scheduling.
- Examples and helper layers need to demonstrate the correct timing model clearly.

## ED-009: The v1 product is a focused driver, not a kitchen sink

Date: 2026-03-19
Status: Accepted

Context:

The reference repository mixes protocol logic, demos, utility printing, and sequencing helpers.

Decision:

- The main library API will focus on the driver itself.
- Utilities and examples may exist, but outside the core API surface.
- The old utilities and sequencers are not part of the new architecture unless deliberately reintroduced later.

Consequences:

- The public library stays smaller and easier to understand.
- Example and utility code must be organized as supporting material, not core product surface.

## ED-010: The first API is generic, not device-specific

Date: 2026-03-19
Status: Accepted

Context:

The protocol is shared across multiple EZO devices, while device-specific helpers are convenience layers rather than fundamental transport behavior.

Decision:

- The first milestone will provide a generic command/response API.
- Typed helpers for pH, EC, RTD, DO, and similar devices are deferred.

Consequences:

- The core stays small and general.
- Device-specific helpers can be added later as thin layers if justified.

## ED-011: The first complete milestone targets both Arduino and Linux

Date: 2026-03-19
Status: Accepted

Context:

The library must be genuinely cross-platform, not merely architected for portability on paper.

Decision:

- The first complete milestone includes Arduino and Linux support.
- Architecture decisions must support both platforms from the start.

Consequences:

- Adapters, examples, and build/test flows must account for both environments.
- Platform-specific work cannot be deferred indefinitely without missing the milestone definition.

## ED-012: Build and packaging are CMake-first

Date: 2026-03-19
Status: Accepted

Context:

The project needs host-side builds and tests while still being usable from Arduino tooling.

Decision:

- The main project layout and build flow will be CMake-first.
- Arduino packaging metadata will be layered on top.
- PlatformIO support is planned and example parity should be maintained when examples are added.

Consequences:

- CMake becomes the primary developer workflow.
- Arduino/PlatformIO packaging work should stay aligned with, not drive, the core layout.

## ED-013: Public numeric values use `double` in the C API

Date: 2026-03-19
Status: Accepted

Context:

The public API needs a numeric type for command formatting and parsing that works cleanly on both host and embedded targets.

Decision:

- The public C API will use `double` for numeric send/parse helpers.
- Embedded targets may still implement `double` with the same precision as `float`, but the public contract remains `double`.

Consequences:

- Host-side users get a conventional numeric API.
- Arduino-class targets still remain compatible even where `double` aliases `float`.

## ED-014: The transport contract uses one transaction primitive in v1

Date: 2026-03-19
Status: Accepted

Context:

The transport interface should be as small as possible while still supporting both Arduino and Linux adapters and deterministic test doubles.

Decision:

- The v1 transport contract uses a single callback for write/read bus transactions.
- Send-only and read-only operations are represented by zero-length read or write sides respectively.

Consequences:

- The core stays simpler and easier to fake in tests.
- If adapter implementation later proves this shape inadequate, the contract must be revised explicitly rather than split ad hoc.

## ED-015: The core will not replicate legacy read-issued state tracking

Date: 2026-03-19
Status: Accepted

Context:

The legacy API uses `issued_read` and `NOT_READ_CMD` to enforce a particular object workflow. That behavior is tied to the old API shape rather than the device protocol itself.

Decision:

- The new core will not track legacy "read was previously issued" object state solely to emulate `NOT_READ_CMD`.
- Caller intent is expressed through explicit API choice and call flow rather than hidden device flags.

Consequences:

- The device object stays smaller and less stateful.
- The new API remains simpler and avoids preserving legacy behavior that does not improve the protocol model.

## Locked Baseline

- Compatibility target: new API, not a drop-in replacement
- Core language: C99
- C++ layer: thin C++11 wrapper over the C core
- Transport model: callbacks/function pointers with caller-owned context
- Execution model: synchronous first
- Memory model: no dynamic allocation in core, caller-owned buffers
- Error model: library errors separated from device status codes
- Timing policy: core does not sleep; timing remains explicit
- v1 scope: focused driver with utilities/examples outside the core API
- Typed helpers in v1: no
- First complete milestone: Arduino and Linux
- Build approach: CMake-first with Arduino metadata and planned PlatformIO support
- Public numeric type: `double`
- Transport callback shape: single write/read transaction primitive
- Legacy read-issued compatibility state: not preserved
