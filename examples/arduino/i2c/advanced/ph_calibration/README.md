# Arduino I2C pH Calibration

This sketch stages pH calibration one step at a time with preview reads and explicit midpoint-first ordering.

## Before You Start

- Use fresh buffer solutions in separate cups. The vendor-recommended simple set is `7.00`, `4.00`, and `10.00`.
- Rinse the probe between buffers so you do not contaminate the next point.
- The sketch defaults to `CALIBRATION_STEP=STEP_STATUS`, so the first upload is inspect-only unless you change it.

## Run It

- Start with `STEP_STATUS` and `APPLY_CHANGES=0` to inspect the current calibration level, temperature compensation, and slope.
- For the midpoint step, set `CALIBRATION_STEP=STEP_MID`, `REFERENCE_PH=7.00`, and leave `APPLY_CHANGES=0` first. Upload and confirm the preview readings are stable.
- Re-upload the same midpoint step with `APPLY_CHANGES=1` to commit it.
- Repeat the same pattern for `STEP_LOW` with `REFERENCE_PH=4.00`, then `STEP_HIGH` with `REFERENCE_PH=10.00`.
- Wait until the readings have stabilized before each apply pass. Vendor guidance is midpoint first, then low, then high.

## Check Result

- Re-run `STEP_STATUS` after the final point and confirm the reported `post_calibration_level` and post-calibration slope output.
- `STEP_MID` is not just another point: if you run midpoint calibration again after low or high, the vendor says the other points are cleared and full calibration must be redone.
- After a full calibration, check the slope once at the end. A healthy probe should usually show acid and base slopes above about `90%` and similar to each other, and the neutral mV offset should stay reasonably close to `0`.
