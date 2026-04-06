# Arduino I2C RTD to pH Temperature Compensation

This sketch continuously reads an RTD circuit, writes that temperature into the pH circuit, and then reads pH.

## Use It

- Set `RTD_I2C_ADDRESS` and `PH_I2C_ADDRESS`.
- Upload the sketch and open Serial Monitor at `115200`.
- This sketch actively updates pH temperature compensation every cycle. Use it only when the RTD probe represents the same liquid temperature as the pH probe.
