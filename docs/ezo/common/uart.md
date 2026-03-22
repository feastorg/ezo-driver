# UART Behavior

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page documents the UART behavior that matters to `ezo_uart_*` and to any future UART adapter work.

## Core Model

Across the current EZO product set, UART is the shipping default mode.

At the repo level, UART is treated as a line-oriented ASCII protocol:

- callers pass command text without terminators
- `ezo_uart_send_command()` appends a single carriage return
- `ezo_uart_read_line()` reads one CR-terminated line
- successful reads return a null-terminated text line

That matches the contract in [`src/ezo_uart.h`](../../src/ezo_uart.h) and the framing logic in [`src/ezo_uart.c`](../../src/ezo_uart.c).

## Response Shapes

UART responses fall into three practical buckets:

1. measurement or query data
2. command acknowledgements such as `*OK`
3. control or state tokens such as reset, wake, sleep, and power-condition markers

The current repo surface now distinguishes:

- `DATA`
- `*OK`
- `*ER`
- `*OV`
- `*UV`
- `*RS`
- `*RE`
- `*SL`
- `*WA`
- `*DONE`

Those tokens remain transport-level classifications. The core still does not interpret them as product behavior.

## Response Sequences

Many UART workflows are not truly one-command, one-line exchanges.

Examples include:

- a data line followed by trailing `*OK`
- a response-code query line such as `?*OK,0` with no trailing `*OK`
- export flows ending in `*DONE`
- memory or recall flows that emit multiple data lines before a final status token
- startup, sleep, wake, reset, and ready lines that can appear between higher-level operations

The repo-level rule is therefore:

- the low-level primitive reads one line
- callers or higher layers consume sequences by reading multiple lines
- typed product helpers may consume both a payload line and a trailing success token when they expose a parsed success result
- setter or admin workflows that only acknowledge success should explicitly consume that terminal token with `ezo_uart_read_ok()`
- `ezo_uart_discard_input()` is the explicit resynchronization tool when abandoning stale or unwanted trailing lines

## Synchronization Rules

The low-level core does not try to infer workflow boundaries across lines.

Higher layers should therefore follow these rules:

- treat startup and power-state lines such as wake, ready, and reset as valid control events, not as transport corruption
- consume or discard stale continuous output before starting a workflow that expects a specific next line
- consume trailing status tokens such as `*OK` or `*DONE`, or explicitly discard them before reusing the transport
- use the raw `ezo_uart_*` layer when the application needs line-by-line ownership instead of the typed helper's normalized success sequence
- treat shipping defaults such as continuous mode enabled or `*OK` enabled as heuristics only, not as a guaranteed runtime state

## Response-Code Bootstrap

Higher layers that depend on response-code-enabled UART flows should bootstrap that state explicitly instead of assuming the shipping default is still active:

- send `ezo_control_send_response_code_query_uart()`
- read the mode with `ezo_control_read_response_code_uart()`
- if disabled, send `ezo_control_send_response_code_set_uart()`
- consume the setter acknowledgement with `ezo_uart_read_ok()`
- only then start typed workflows that require trailing `*OK`

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
- collapse multi-line command workflows into one implicit read helper
- infer CSV schemas for multi-output products

That keeps the UART core transport-focused. If the repo later grows typed UART helpers, they should sit above this layer and be product-aware.
