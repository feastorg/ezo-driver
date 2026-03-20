# UART Behavior

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page documents the UART behavior that matters to `ezo_uart_*` and to any future UART adapter work.

## Core Model

Across the current EZO product set, UART is the shipping default mode.

At the repo level, UART is treated as a line-oriented ASCII protocol:

- callers pass command text without terminators
- `ezo_uart_send_command()` appends a single carriage return
- `ezo_uart_read_response()` reads until the next carriage return
- successful reads return a null-terminated text line

That matches the contract in [`src/ezo_uart.h`](../../src/ezo_uart.h) and the framing logic in [`src/ezo_uart.c`](../../src/ezo_uart.c).

## Response Shapes

UART responses fall into three practical buckets:

1. measurement or query data
2. command acknowledgements such as `*OK`
3. control or state tokens such as reset, wake, sleep, and power-condition markers

The current repo surface models only:

- `DATA`
- `*OK`
- `*ER`

That is enough for the present baseline, but vendor documentation for multiple products also defines additional control tokens such as over-voltage, under-voltage, reset, ready, sleep, and wake indications. Today those lines are treated as generic data by the core.

## Single Read And Continuous Read

The transport behavior is stable even though the measurement cadence differs by product:

- one-shot reads return one logical reading after the caller waits long enough
- continuous mode produces recurring text lines until disabled

This repo does not hide that timing or cadence. Callers own:

- when to wait
- when to poll
- when to discard stale input

The optional `discard_input` transport hook exists for exactly that reason.

## Timing

The generic timing hints in [`src/ezo.c`](../../src/ezo.c) are conservative defaults, not a product-accurate UART timing database.

For example:

- several products read at roughly one second in UART mode
- EC and DO are faster than that for one-shot reads
- some non-measurement commands complete much faster
- calibration and reboot-triggering commands can take longer

Use product pages in `../products/` for high-level timing guidance, and the official vendor documentation for exact command timing.

## Repo Implications

The current UART layer intentionally does not:

- expose raw byte frames
- hide retries or delays
- normalize product-specific control tokens
- infer CSV schemas for multi-output products

That keeps the UART core transport-focused. If the repo later grows typed UART helpers, they should sit above this layer and be product-aware.
