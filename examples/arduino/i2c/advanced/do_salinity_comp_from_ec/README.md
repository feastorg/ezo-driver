# Arduino I2C EC to D.O. Salinity Compensation

This sketch continuously reads salinity from an EC circuit, writes that value into the D.O. circuit, and then reads D.O.

## Use It

- Set `EC_I2C_ADDRESS` and `DO_I2C_ADDRESS`.
- Upload the sketch and open Serial Monitor at `115200`.
- Make sure EC salinity output is enabled first. This sketch actively updates D.O. salinity compensation every cycle.
