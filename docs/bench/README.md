# Bench Records

Results of running ezo-driver against real EZO circuits, one file per bench session. The host tests (`tests/`, `bindings/python/tests`) cover the library's logic; these records cover what it does with a circuit on the other end.

A record names the library version and where it came from, the controller and transport, each circuit's product, firmware and address, and the probes. It lists each step with what was expected, what was seen, and pass, fail or pending. It attaches the raw output, and it says what was not checked.

| Date | Record | Library | Circuits | Result |
| --- | --- | --- | --- | --- |
| 2026-11-01 | [pH and DO circuits over Linux I2C](2026-11-01-ezo-ph-do.md) | 0.8.0 (PyPI), Python | pH 2.15, DO 2.17 | pass; Sleep on pH 2.15 does not wake (#33) |
