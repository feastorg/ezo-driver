"""Bench helper for #48: the same checks over Linux I2C and UART, through
ezo-driver's Python binding. Install the driver from the commit under test.

  ezo_bench48.py LINK PRODUCT COMMAND [ARGS]

LINK is i2c:0x63 (bus 1) or uart:/dev/serial0 (9600 baud).
PRODUCT is ph, ec, do, o2 or co2.

  info | status          identity and firmware; restart code and voltage
  read N                 N readings through the typed helpers
  outq                   enabled outputs (ec, do, o2, co2)
  outset NAME 0|1        change an output, e.g. PPT 1 on o2 (writes the circuit)
  rt TEMP                reading with temperature compensation, then restores T
  warm SECONDS           co2 right after power-up: R in a loop, printing replies
  sleep                  Sleep, wait 5 s, then wake with i
  slope                  pH probe slope
  listen SECONDS         UART: print every line the circuit sends
  raw TEXT SECONDS       UART: send TEXT, print every line for SECONDS
  cal-ec POINT VALUE     EC calibration at the driver's hint, then the raw reply;
                         POINT is DRY, SINGLE_POINT, LOW_POINT or HIGH_POINT
                         (writes stored calibration; DRY ignores VALUE)
  to-uart BAUD           from I2C, reboot the circuit in UART mode
  to-i2c ADDR            from UART, reboot the circuit in I2C mode

UART circuits ship with continuous readings on, and those lines would land in
the middle of a command's reply. Run `listen 3`, then `raw C,0 2`, before the
typed commands. A UART command other than listen and raw discards pending input
first, which clears old lines but not ones that arrive during the command.

UART run:   to-uart 9600; listen 3; raw C,0 2; info; status; read 3; outq;
            raw R 2; raw RT,25 3; rt 25; to-i2c ADDR
CO2 warm:   power-cycle, then `warm 15` at once. Over UART this prints whether
            *OK follows *warm. Power-cycle again and `read 15` for the helper.
Two values: outset NAME 1, read 3, raw R 2 (UART), outset NAME 0.
"""
import sys
import time
from datetime import datetime

import ezo_driver
from ezo_driver import (
    EzoDeviceStatusError,
    EzoError,
    EzoTransportError,
    LinuxI2CDevice,
    LinuxUARTDevice,
    ProductId,
    co2,
    control,
    do,
    ec,
    o2,
    ph,
)
from ezo_driver.enums import (
    CO2OutputMask,
    CommandKind,
    DOOutputMask,
    ECCalibrationPoint,
    ECOutputMask,
    O2OutputMask,
)

BUS = 1
BAUD = 9600
PRODUCTS = {
    "ph": (ProductId.PH, ph, None),
    "ec": (ProductId.EC, ec, ECOutputMask),
    "do": (ProductId.DO, do, DOOutputMask),
    "o2": (ProductId.O2, o2, O2OutputMask),
    "co2": (ProductId.CO2, co2, CO2OutputMask),
}


def say(*a):
    print(datetime.now().strftime("%H:%M:%S.%f")[:-3], *a, flush=True)


def wait(ms):
    time.sleep(ms / 1000)


def exc(e):
    extra = ""
    if isinstance(e, EzoTransportError):
        extra = f" errno={e.errno} bus={e.bus} address={e.address}"
    if isinstance(e, EzoDeviceStatusError):
        extra = f" status={e.status.name}"
    return f"{type(e).__name__}({e}){extra}"


def open_link(spec):
    kind, _, target = spec.partition(":")
    if kind == "i2c":
        return "i2c", LinuxI2CDevice(bus=BUS, address=int(target, 0))
    if kind == "uart":
        return "uart", LinuxUARTDevice(target, BAUD, read_timeout_ms=1500)
    sys.exit(f"unknown link {spec}")


class Bench:
    def __init__(self, t, dev, prod):
        self.t, self.dev = t, dev
        self.pid, self.mod, self.outputs = PRODUCTS[prod]

    def fn(self, mod, name):
        return getattr(mod, f"{name}_{self.t}")

    def query(self, mod, send, read, *send_args):
        wait(self.fn(mod, send)(self.dev, *send_args))
        return self.fn(mod, read)(self.dev)

    def control(self, send, read):
        return self.query(control, send, read, self.pid)

    def mask(self):
        if self.outputs is None:
            return None
        return self.query(self.mod, "send_output_query", "read_output_config").enabled_mask

    def read_one(self, mask):
        wait(self.fn(self.mod, "send_read")(self.dev))
        read = self.fn(self.mod, "read_response")
        return read(self.dev) if mask is None else read(self.dev, mask)

    def ack(self, ms):
        wait(ms)
        if self.t == "uart":
            self.dev.read_ok()

    def capture(self, seconds):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            try:
                kind, text = self.dev.read_line()
                say(f"{UARTName(kind)} {text!r}")
            except EzoError as e:
                say("no line:", exc(e))

    def need_uart(self, cmd):
        if self.t != "uart":
            sys.exit(f"{cmd} is UART only")


def UARTName(kind):
    return getattr(kind, "name", kind)


def run(b, cmd, args):
    dev = b.dev
    if cmd == "listen":
        b.need_uart(cmd)
        b.capture(float(args[0]))
    elif cmd == "raw":
        b.need_uart(cmd)
        dev.send_command(args[0], CommandKind.GENERIC)
        say("sent", repr(args[0]))
        b.capture(float(args[1]))
    elif cmd == "info":
        say(b.control("send_info_query", "read_info"))
    elif cmd == "status":
        say(b.control("send_status_query", "read_status"))
    elif cmd == "read":
        mask = b.mask()
        say("outputs", mask)
        for _ in range(int(args[0])):
            try:
                say(b.read_one(mask))
            except EzoError as e:
                say(exc(e))
    elif cmd == "outq":
        say(b.query(b.mod, "send_output_query", "read_output_config"))
    elif cmd == "outset":
        ms = b.fn(b.mod, "send_output_set")(dev, b.outputs[args[0].upper()], args[1] == "1")
        b.ack(ms)
        say("set", args[0], args[1])
        say(b.query(b.mod, "send_output_query", "read_output_config"))
    elif cmd == "rt":
        before = b.query(b.mod, "send_temperature_query", "read_temperature")
        say("T before:", before)
        mask = b.mask()
        wait(b.fn(b.mod, "send_read_with_temp_comp")(dev, float(args[0])))
        read = b.mod.read_response_with_temp_comp_uart if b.t == "uart" else b.mod.read_response_i2c
        say("RT:", read(dev) if mask is None else read(dev, mask))
        say("T after:", b.query(b.mod, "send_temperature_query", "read_temperature"))
        b.ack(b.fn(b.mod, "send_temperature_set")(dev, before.temperature_c))
        say("T restored:", b.query(b.mod, "send_temperature_query", "read_temperature"))
    elif cmd == "warm":
        mask = b.mask() if b.t == "i2c" else None
        end = time.monotonic() + float(args[0])
        while time.monotonic() < end:
            if b.t == "uart":
                dev.send_command("R", CommandKind.READ)
                say("sent 'R'")
                b.capture(1.5)
            else:
                try:
                    say(b.read_one(mask))
                except EzoError as e:
                    say(exc(e))
    elif cmd == "sleep":
        b.fn(control, "send_sleep")(dev, b.pid)
        say("Sleep sent; LED should go off. 5 s")
        time.sleep(5)
        for i in range(2):
            try:
                say(f"wake try {i + 1}:", b.control("send_info_query", "read_info"))
                break
            except EzoError as e:
                say(f"wake try {i + 1}:", exc(e))
    elif cmd == "slope":
        say(b.query(ph, "send_slope_query", "read_slope"))
    elif cmd == "cal-ec":
        say("before:", b.query(ec, "send_calibration_query", "read_calibration_status"))
        ms = b.fn(ec, "send_calibration")(dev, ECCalibrationPoint[args[0].upper()], float(args[1]))
        say(f"sent {args[0]} {args[1]}; waiting the {ms} ms hint")
        wait(ms)
        say("reply:", dev.read_response() if b.t == "i2c" else dev.read_line())
        if b.t == "uart":
            dev.discard_input()
        say("after:", b.query(ec, "send_calibration_query", "read_calibration_status"))
    elif cmd == "to-uart":
        if b.t != "i2c":
            sys.exit("to-uart runs from an I2C link")
        b.fn(control, "send_switch_to_uart")(dev, b.pid, int(args[0]))
        say("sent; the circuit reboots in UART mode (green LED)")
    elif cmd == "to-i2c":
        b.need_uart(cmd)
        b.fn(control, "send_switch_to_i2c")(dev, b.pid, int(args[0], 0))
        say("sent; the circuit reboots in I2C mode (blue LED)")
    else:
        sys.exit(f"unknown command {cmd}")


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    spec, prod, cmd, args = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:]
    say(f"ezo-driver {ezo_driver.__version__} {spec} {prod} {cmd} {' '.join(args)}")
    t, dev = open_link(spec)
    with dev:
        if t == "uart" and cmd not in ("listen", "raw"):
            dev.discard_input()
        try:
            run(Bench(t, dev, prod), cmd, args)
        except EzoError as e:
            say(exc(e))
            sys.exit(1)


if __name__ == "__main__":
    main()
