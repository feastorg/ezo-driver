# EZO Source Policy

> Notice: This is an original repo policy document informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Goal

The `docs/ezo/` subtree should preserve useful product knowledge without copying vendor documentation into this repo.

## Authoritative Inputs

The documentation work in this repo is based on:

1. the code and tracked docs inside `ezo-driver`
2. vendor-derived working notes kept outside this repo
3. explicit repo decisions already recorded in tracked `docs/`

The authoritative public source for device operation remains Atlas Scientific's own documentation.

## Required Practices

When adding or updating EZO docs in this repo:

1. write fresh summaries in repo language
2. organize material around repo concerns, not vendor section order
3. keep common transport behavior separate from product behavior
4. summarize only the facts needed to use, extend, or test this driver
5. point readers to official vendor documentation when a full procedure matters

## Prohibited Practices

Do not:

1. copy vendor tables, diagrams, or page structure
2. reproduce long command catalogs or full quick-reference pages
3. restate full calibration, recovery, or hardware-switch procedures
4. present these docs as official Atlas Scientific documentation
5. move vendor source material into this repo as "docs"

## Acceptable Use Of Vendor Facts

Using factual product information is expected. The safe form is a curated summary such as:

- default transport mode
- default I2C address
- broad timing shape
- output model
- command families present on a product
- protocol deltas that affect driver design

Prefer prose and short lists over copied tabular layouts.

## Citation And Provenance Style

Each page in `docs/ezo/` should carry a short notice that it is:

- repo-authored
- informed by vendor documentation
- not an official vendor manual

If a detail is too specific to summarize cleanly without recreating the original material, omit the detail and direct readers to the official vendor documentation instead.

## Maintenance Rule

When a vendor update changes device behavior:

1. update the relevant repo summary
2. keep the wording original
3. update only the repo-relevant conclusion, not the full vendor explanation

If a future change would force large copied excerpts to stay accurate, that material does not belong in this repo.
