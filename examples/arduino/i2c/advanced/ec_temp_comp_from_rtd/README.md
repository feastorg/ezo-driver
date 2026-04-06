# Arduino I2C RTD to EC Temperature Compensation

This sketch continuously reads an RTD circuit, writes that temperature into the EC circuit, and then reads EC.

## Use It

- Set `RTD_I2C_ADDRESS` and `EC_I2C_ADDRESS`.
- Upload the sketch and open Serial Monitor at `115200`.
- This sketch actively updates EC temperature compensation every cycle. Use it only when the RTD probe represents the same liquid temperature as the EC probe.
