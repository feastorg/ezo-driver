# Linux I2C Typed Reads

These examples do one simple typed read per supported product.

## Examples

- `read_ph.c`: pH typed read
- `read_orp.c`: ORP typed read
- `read_ec.c`: EC typed read with explicit output-config ownership
- `read_do.c`: D.O. typed read with explicit output-config ownership
- `read_rtd.c`: RTD typed read with an explicit scale query
- `read_hum.c`: HUM typed read with explicit output-config ownership

## Use It

1. Build with `cmake --preset host-linux-debug` and `cmake --build --preset host-linux-debug --parallel`.
2. Run the matching binary from `build/host-linux-debug/`, for example:
   - `./build/host-linux-debug/ezo_linux_i2c_read_ph_example [device_path] [address]`
   - `./build/host-linux-debug/ezo_linux_i2c_read_do_example [device_path] [address]`
3. Use the product default address when possible:
   - pH `99`
   - ORP `98`
   - EC `100`
   - D.O. `97`
   - RTD `102`
   - HUM `111`

## Notes

- These are the smallest parsed-read paths. They intentionally do not mutate configuration.
- Move to `../advanced/` for calibration, compensation, or control workflows.
