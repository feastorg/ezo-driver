# Product Onboarding

## Purpose

This document is the maintainer checklist for adding support for another EZO family.

It exists to keep future product work mechanical. New families should extend the repo by following the same process, not by reopening architecture decisions.

## Preconditions

Do not start implementation until all of these are true:

1. vendor evidence for the family exists outside this repo and is strong enough to support driver work
2. the family has been reviewed against the existing transport and product model
3. the intended initial support tier is explicit before code is written

## Support Tiers

`METADATA`

- product ID and short-code identification
- timing metadata
- capability and command-family classification
- canonical schema facts where they are stable enough to record
- no typed measurement or product-helper claim

`TYPED_READ`

- metadata coverage
- typed measurement reads for the main acquisition path
- product-specific parse and command helpers needed for that path
- partial configuration support is acceptable if the docs say so clearly

`FULL`

- typed reads
- shared control/admin coverage where the family participates
- calibration-transfer coverage where the family supports it
- advanced product-specific helpers that are part of the intended public surface

## Required Decisions Up Front

Before implementation, record these conclusions:

1. scalar-output or configurable multi-output
2. fixed schema, primary-only schema, or query-required schema
3. calibration-transfer support: yes or no
4. shared control-plane participation: yes or no
5. advanced helpers in scope now vs intentionally deferred
6. initial tier claim: `METADATA`, `TYPED_READ`, or `FULL`

If any answer would force a change to the transport architecture or the current product/module split, stop and update the tracked architecture docs first.

## File-Level Checklist

Every new family should clear this checklist in order:

1. Review vendor evidence and write repo conclusions.
   Target files:
   - `docs/ezo/products/<family>.md`
   - `docs/ezo/common/*.md` only if shared protocol conclusions changed

2. Add product identity and metadata.
   Target files:
   - `src/ezo_product.h`
   - `src/ezo_product.c`

3. Add schema and parse facts where needed.
   Target files:
   - `src/ezo_schema.h`
   - `src/ezo_schema.c`
   - `src/ezo_parse.h`
   - `src/ezo_parse.c`

4. Add the product module.
   Target files:
   - `src/ezo_<family>.h`
   - `src/ezo_<family>.c`

5. Add or update tests.
   Minimum targets:
   - `tests/test_ezo_product.c`
   - `tests/test_ezo_<family>.c`
   - shared parser/schema tests if the family forced shared behavior changes

6. Add an example only if the tier justifies it.
   Typical targets:
   - `examples/linux/i2c/typed/read_<family>.c`
   - `examples/linux/uart/typed/read_<family>.c`
   - Arduino examples only when they add real coverage value

7. Update public repo docs.
   Target files:
   - `docs/support-matrix.md`
   - `README.md`
   - `CHANGELOG.md`

## Validation Bar

Minimum validation for any new family:

1. metadata lookup and `i`-response handling are covered where applicable
2. parser behavior is covered for the family's real payload shapes
3. I2C and UART helpers are covered when the tier claims both
4. support-tier claims in docs match the actual exported surface
5. examples compile on the CI paths they are supposed to represent

Recommended validation commands:

```sh
cmake -S . -B build -DEZO_BUILD_TESTS=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
git diff --check
```

If example or packaging surfaces changed, rerun the relevant example or install validation too.

## Tier Rules

Do not overclaim maturity:

1. use `METADATA` when the repo can identify the family but does not yet provide typed reads
2. use `TYPED_READ` when the acquisition path is solid but broader operational coverage is intentionally partial
3. use `FULL` only when implementation, tests, and docs all reflect the intended public surface

If a family cannot clear a higher tier yet, land it at the lower tier explicitly instead of hiding the gap.

## Current Constraint

The current repo only carries vendor source material for the initial six supported families. Until new evidence is added outside this repo, this document is the close-out for growth readiness rather than a mandate to ship another family immediately.
