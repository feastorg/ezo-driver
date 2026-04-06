# Arduino I2C Read EC

This sketch performs one typed EC read and first queries the active output mask so the response is decoded correctly.

## Use It

- Set `EC_I2C_ADDRESS`.
- Upload the sketch and open Serial Monitor at `115200`.
- Move to `../../advanced/ec_workflow/ec_workflow.ino` if you need probe K, TDS factor, salinity output, or calibration state.
