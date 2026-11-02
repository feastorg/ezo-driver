"""EZO bench helper for a Pi controller: ezo-driver's Python binding over
/dev/i2c-1. Read-only apart from `sleep` and `led`; never calibrates, never
factory-resets, never changes the address or protocol.

  ezo_bench.py info ADDR PRODUCT        i query: product code, firmware
  ezo_bench.py status ADDR PRODUCT      Status: restart code, supply voltage
  ezo_bench.py led ADDR PRODUCT [0|1]   query, or set, the LED
  ezo_bench.py read ADDR PRODUCT N      N readings, one per wait (ph or do)
  ezo_bench.py tempq ADDR PRODUCT       temperature compensation in force
  ezo_bench.py calq ADDR PRODUCT        calibration status (query only)
  ezo_bench.py early ADDR PRODUCT       read before the wait: expect NOT_READY
  ezo_bench.py nodata ADDR PRODUCT      a read after one already read: expect NO_DATA
  ezo_bench.py sleep ADDR PRODUCT       Sleep, then the i query wakes it
  ezo_bench.py noaddr ADDR              open and read at an address with nothing on it
  ezo_bench.py watch ADDR PRODUCT SECONDS   a reading per read cycle, to watch it settle
  ezo_bench.py slope ADDR ph            pH probe slope: acid and base response, % of ideal
  ezo_bench.py compq ADDR do           DO temperature, pressure and salinity compensation
  ezo_bench.py cal ADDR ph mid|low|high VALUE   one pH calibration point (writes the circuit)
  ezo_bench.py cal ADDR do zero|air     one DO calibration point (writes the circuit)

PRODUCT is ph or do. ADDR in hex (0x63) or decimal. `cal` is the only
command that changes stored calibration; it prints the status before and after.
"""
import sys
import time
from datetime import datetime

import ezo_driver
from ezo_driver import (
    EzoDeviceStatusError,
    EzoError,
    EzoNotReadyError,
    EzoTransportError,
    LinuxI2CDevice,
    ProductId,
    control,
    do,
    ph,
)
from ezo_driver.enums import DOCalibrationPoint, PHCalibrationPoint

BUS = 1
PRODUCTS = {"ph": (ProductId.PH, ph), "do": (ProductId.DO, do)}


def stamp():
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]


def say(*a):
    print(stamp(), *a, flush=True)


def wait(ms):
    time.sleep(ms / 1000)


def exc(e):
    extra = ""
    if isinstance(e, EzoTransportError):
        extra = f" errno={e.errno} bus={e.bus} address={e.address}"
    if isinstance(e, EzoDeviceStatusError):
        extra = f" status={e.status.name}"
    return f"{type(e).__name__}({e}){extra}"


def do_mask(dev):
    wait(do.send_output_query_i2c(dev))
    return do.read_output_config_i2c(dev).enabled_mask


def read_one(dev, prod, mask=None):
    mod = PRODUCTS[prod][1]
    wait(mod.send_read_i2c(dev))
    return mod.read_response_i2c(dev, mask) if prod == "do" else mod.read_response_i2c(dev)


def main():
    cmd, args = sys.argv[1], sys.argv[2:]
    addr = int(args[0], 0)
    say(f"ezo-driver {ezo_driver.__version__} bus {BUS} address {addr:#04x} {cmd}")
    if cmd == "noaddr":
        try:
            dev = LinuxI2CDevice(BUS, addr)
            wait(control.send_info_query_i2c(dev, ProductId.PH))
            say("read", control.read_info_i2c(dev))
        except EzoError as e:
            say(exc(e))
        return
    prod = args[1]
    pid, mod = PRODUCTS[prod]
    with LinuxI2CDevice(bus=BUS, address=addr) as dev:
        try:
            if cmd == "info":
                wait(control.send_info_query_i2c(dev, pid))
                say(control.read_info_i2c(dev))
            elif cmd == "status":
                wait(control.send_status_query_i2c(dev, pid))
                say(control.read_status_i2c(dev))
            elif cmd == "led":
                if len(args) > 2:
                    wait(control.send_led_set_i2c(dev, pid, args[2] == "1"))
                    say("LED set", args[2])
                wait(control.send_led_query_i2c(dev, pid))
                say(control.read_led_i2c(dev))
            elif cmd == "read":
                mask = do_mask(dev) if prod == "do" else None
                if mask is not None:
                    say("DO outputs", mask)
                for _ in range(int(args[2])):
                    say(read_one(dev, prod, mask))
            elif cmd == "tempq":
                wait(mod.send_temperature_query_i2c(dev))
                say(mod.read_temperature_i2c(dev))
            elif cmd == "calq":
                wait(mod.send_calibration_query_i2c(dev))
                say(mod.read_calibration_status_i2c(dev))
            elif cmd == "early":
                mask = do_mask(dev) if prod == "do" else None
                ms = mod.send_read_i2c(dev)
                say(f"read sent, wait {ms} ms; reading after 0 ms")
                try:
                    r = mod.read_response_i2c(dev, mask) if prod == "do" else mod.read_response_i2c(dev)
                    say("no error:", r)
                except EzoNotReadyError as e:
                    say("expected:", exc(e))
                wait(ms)
                r = mod.read_response_i2c(dev, mask) if prod == "do" else mod.read_response_i2c(dev)
                say("after the wait:", r)
            elif cmd == "nodata":
                mask = do_mask(dev) if prod == "do" else None
                say("first:", read_one(dev, prod, mask))
                try:
                    r = mod.read_response_i2c(dev, mask) if prod == "do" else mod.read_response_i2c(dev)
                    say("second read, no error:", r)
                except EzoDeviceStatusError as e:
                    say("second read:", exc(e))
            elif cmd == "sleep":
                control.send_sleep_i2c(dev, pid)
                say("Sleep sent; LED should go off. 5 s")
                time.sleep(5)
                for i in range(2):
                    try:
                        wait(control.send_info_query_i2c(dev, pid))
                        say(f"wake try {i + 1}:", control.read_info_i2c(dev))
                        break
                    except EzoError as e:
                        say(f"wake try {i + 1}:", exc(e))
            elif cmd == "watch":
                mask = do_mask(dev) if prod == "do" else None
                end = time.monotonic() + float(args[2])
                while time.monotonic() < end:
                    say(read_one(dev, prod, mask))
            elif cmd == "slope":
                wait(ph.send_slope_query_i2c(dev))
                say(ph.read_slope_i2c(dev))
            elif cmd == "compq":
                for q, r in ((do.send_temperature_query_i2c, do.read_temperature_i2c),
                             (do.send_pressure_query_i2c, do.read_pressure_i2c),
                             (do.send_salinity_query_i2c, do.read_salinity_i2c)):
                    wait(q(dev))
                    say(r(dev))
            elif cmd == "cal":
                wait(mod.send_calibration_query_i2c(dev))
                say("before:", mod.read_calibration_status_i2c(dev))
                if prod == "ph":
                    point = PHCalibrationPoint[args[2].upper()]
                    ms = ph.send_calibration_i2c(dev, point, float(args[3]))
                    say(f"sent Cal,{args[2]},{float(args[3]):.2f}; wait {ms} ms")
                else:
                    point = {"zero": DOCalibrationPoint.ZERO, "air": DOCalibrationPoint.ATMOSPHERIC}[args[2]]
                    ms = do.send_calibration_i2c(dev, point)
                    say(f"sent {do.build_calibration_command(point)}; wait {ms} ms")
                wait(ms)
                wait(mod.send_calibration_query_i2c(dev))
                say("after:", mod.read_calibration_status_i2c(dev))
                mask = do_mask(dev) if prod == "do" else None
                for _ in range(3):
                    say(read_one(dev, prod, mask))
            else:
                sys.exit(f"unknown command {cmd}")
        except EzoError as e:
            say(exc(e))
            sys.exit(1)


if __name__ == "__main__":
    main()
