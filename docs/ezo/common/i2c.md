# I2C Behavior

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page documents the I2C behavior that matters to `ezo_send_command()`, `ezo_read_response()`, and future I2C adapters.

## Core Model

The repo treats EZO I2C as an addressed command-and-response device model:

1. write an ASCII command to a device address
2. wait long enough for the device to process it
3. perform a later addressed read to fetch the response frame

That is why the transport contract is a single injected `write_then_read(...)` callback in [`src/ezo_i2c.h`](../../src/ezo_i2c.h).

## Response Frame Shape

At the repo level, an I2C response frame is interpreted as:

1. one status byte
2. an ASCII payload when present
3. a terminating null byte in the text-oriented path

The current implementation centralizes this in [`src/ezo_i2c.c`](../../src/ezo_i2c.c).

## Status Byte Mapping

The core maps the vendor status model into `ezo_device_status_t`:

- `1` -> success
- `2` -> fail / syntax-style error
- `254` -> not ready
- `255` -> no data

Unknown status bytes are treated as protocol errors.

An important contract point is that a valid device status is separate from the library result code:

- `EZO_OK` means the transport and frame decode succeeded
- the device status tells the caller whether the sensor had data, was ready, or rejected the request

## Timing And Read Discipline

I2C callers own timing in the same way UART callers do.

The generic timing hints in [`src/ezo.c`](../../src/ezo.c) are only broad defaults. Vendor documentation shows product-specific and command-specific delays, so the safest model is:

- use repo timing hints as fallback
- use product knowledge when you have it
- do not assume one wait time is correct for every EZO board

If a high-level product summary and a protocol-specific command table disagree slightly, prefer the product-aware or command-aware interpretation over the generic fallback.

`NOT_READY` is a normal part of the protocol, not a transport failure.

## Commands With No Useful Immediate Response

Some command families can reboot the board, change protocol state, or enter sleep. In practice that means a caller may see:

- no useful follow-up payload
- a need to re-establish the expected mode
- a stale response if old bytes were not drained in an adapter layer

Those cases belong to product or protocol-control logic above the raw I2C transport surface.

## Repo Implications

The current I2C layer intentionally keeps these boundaries:

- send and read are separate operations
- text reads and raw reads are separate operations
- caller code inspects `ezo_device_status_t`
- the core does not auto-retry `NOT_READY`

That is the right baseline for a driver library. Future product-aware helpers should build on top of these explicit primitives rather than hiding them.
