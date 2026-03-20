# Mode Switching

> Notice: This page is an original repo-authored summary informed by Atlas Scientific documentation. It is not an official Atlas Scientific manual.

## Purpose

This page centralizes the repo-facing behavior around switching between UART and I2C.

## Default Pattern

Across the current products documented in this repo, UART is the default shipping mode and I2C is an alternate operating mode.

That matters for examples and onboarding:

- a newly received board may not answer on I2C until it has been switched
- a board already used in a project may be in either mode

## Software Switching

The vendor command families are broadly consistent:

- UART-side flows use an `I2C,<address>` style command to switch into I2C mode
- I2C-side flows use the UART baud-rate family to switch back into UART mode

The exact operational sequence is product-defined, but the repo-level rule is stable: mode-switch commands are control-plane operations, not normal measurement commands.

## Protocol Lock

The `Plock` family exists specifically to block mode changes.

For repo purposes, assume:

- protocol lock can prevent switching by software command
- protocol lock can also prevent hardware-assisted switching paths
- callers should not treat a failed mode change as a transport bug until lock state has been considered

## Reboot And Response Expectations

Mode changes, factory reset, and some import flows can reboot the board.

That means callers should expect combinations of:

- immediate acknowledgement and then reboot
- no response on the old transport after the change
- a need to reconnect using the new mode
- transient reset or ready indications on UART-capable paths

The current core does not abstract this. It is a caller concern.

## Manual Hardware Switching

The official vendor documentation includes hardware-level procedures for forcing a mode change. Those procedures are intentionally not reproduced here.

Repo docs only rely on the higher-level conclusion:

- hardware switching exists
- it is product-sensitive
- protocol lock can interfere with it

Use the official vendor documentation for the actual electrical procedure.

## Repo Implications

Mode switching should remain outside the current generic helper surface.

If the repo later adds explicit mode-management helpers, they should:

1. live above the raw transport layers
2. model reboot and reconnect as part of the operation
3. surface protocol-lock failures clearly
