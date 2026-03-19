# Master Plan

## Objective

Deliver a clean cross-platform EZO I2C driver with:

- a C99 core
- a thin C++11 wrapper
- Arduino and Linux adapters
- tests and examples
- packaging that supports both host and Arduino workflows

## Planning Assumptions

- the protocol behavior in `_reference/` is the on-wire reference
- backward compatibility with the legacy API is not required
- the first complete milestone must support both Arduino and Linux
- the core API remains generic in the first milestone

## Workstreams

### 1. Product definition

Outputs:

- vision document
- architecture document
- decision log
- API design notes

Purpose:

- lock intent and boundaries before implementation

### 2. Core API and data model

Outputs:

- public C headers
- internal core module boundaries
- result and status model
- timing model definition

Purpose:

- define the stable implementation target before adapters are built

### 3. Core implementation

Outputs:

- command formatting logic
- response parsing logic
- device lifecycle/state handling
- error/status reporting

Purpose:

- create the portable protocol engine

### 4. Transport and adapters

Outputs:

- transport contract
- Arduino adapter
- Linux adapter
- mock/fake transport

Purpose:

- make the core usable across environments and testable without hardware

### 5. C++ wrapper

Outputs:

- C++11 wrapper over the C core
- wrapper examples or smoke coverage

Purpose:

- expose an ergonomic C++ surface without duplicating logic

### 6. Testing and validation

Outputs:

- unit tests against fake transport
- adapter smoke tests where practical
- example build validation

Purpose:

- prove correctness before and during platform integration

### 7. Packaging and examples

Outputs:

- CMake build
- Arduino library metadata
- PlatformIO support
- focused examples for Arduino and Linux

Purpose:

- make the library consumable and demonstrable

## Sequencing

### Phase 0: Documentation and scope lock

Deliverables:

- decision log
- vision doc
- architecture doc
- master plan

Exit criteria:

- scope, boundaries, and milestone definition are clear enough to design the public API

### Phase 1: Public API design

Deliverables:

- C API draft
- C++ wrapper shape draft
- transport contract draft
- error and timing model definitions

Exit criteria:

- the implementation can proceed without unresolved architectural ambiguity

### Phase 2: Core implementation and tests

Deliverables:

- working C core
- fake transport
- unit tests for command and response behavior

Exit criteria:

- protocol logic is validated independently of hardware

### Phase 3: Platform adapters

Deliverables:

- Arduino adapter
- Linux adapter
- adapter integration tests or smoke checks

Exit criteria:

- the same core works on both target platforms

### Phase 4: C++ wrapper and examples

Deliverables:

- C++11 wrapper
- minimal Arduino example
- minimal Linux example

Exit criteria:

- both language surfaces are usable with documented integration patterns

### Phase 5: Packaging and release readiness

Deliverables:

- stable CMake layout
- Arduino packaging metadata
- PlatformIO support
- contributor and usage documentation

Exit criteria:

- the first complete milestone is ready for external consumption

## Milestone Definition

The first complete milestone includes:

- generic C99 core driver
- thin C++11 wrapper
- Arduino adapter
- Linux adapter
- fake-transport unit tests
- focused examples
- CMake build
- Arduino metadata
- PlatformIO support

## Immediate Next Deliverables

Before code is written, the next design artifacts should be:

1. a public C API draft
2. a transport contract draft
3. an error/status model draft
4. a timing semantics draft
5. a repository layout and build skeleton plan

## Key Risks

1. Ambiguous transport contract
   If the transport interface is too broad or too narrow, both adapters and tests will suffer.

2. Timing semantics drift
   If command timing rules are not modeled clearly, examples and adapters will encode inconsistent behavior.

3. Scope growth
   Pulling utilities, typed helpers, or async behavior into v1 too early will slow core delivery.

4. Packaging drift across ecosystems
   CMake, Arduino metadata, and PlatformIO support must describe the same library layout.

## Risk Mitigations

1. Design the C API and transport contract before implementation.
2. Validate the core with a fake transport before hardware integration.
3. Keep examples minimal and purpose-built.
4. Treat `_reference/` as reference material only, not a partial implementation to port wholesale.

## Done Criteria for the Planning Stage

Planning is complete when:

- the decision log is accepted
- the vision is clear
- the architecture is explicit
- the milestone sequence is defined
- the API design can begin without reopening foundational decisions
