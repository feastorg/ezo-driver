# Arduino I2C Full D.O. Compensation Chain

This sketch continuously drives D.O. compensation from three inputs: RTD temperature, EC salinity, and the fixed `PRESSURE_KPA` constant.

## Use It

- Set `RTD_I2C_ADDRESS`, `EC_I2C_ADDRESS`, `DO_I2C_ADDRESS`, and `PRESSURE_KPA`.
- Upload the sketch and open Serial Monitor at `115200`.
- Make sure the RTD and EC probes represent the same environment as the D.O. probe. This sketch actively updates all three D.O. compensation inputs every cycle.
