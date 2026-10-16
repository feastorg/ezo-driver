"""Run every Python example in bindings/python/README.md against the fakes.

The fake circuits only answer a command once the example sleeps after
sending it, the way a real circuit is still busy until the returned delay has
passed. An example that reads before it waits gets no response and fails.
"""

from __future__ import annotations

import re
import threading

import pytest

import ezo_driver
from ezo_driver import DeviceStatus

from ._fakes import source_dir
from ._support import FakeI2CDevice, FakeUARTDevice, i2c_response, uart_lines

# Replies per I2C address and command, as a circuit would give them.
I2C_REPLIES = {
    0x63: {b"i": "?i,pH,2.16", b"r": "7.15"},
    0x64: {b"i": "?i,EC,2.16", b"O,?": "?O,EC,S", b"r": "842,0.42"},
}
UART_REPLIES = {
    b"i": ("?i,pH,2.16", "*OK"),
    b"r": ("7.15", "*OK"),
}


def readme_examples() -> list[str]:
    text = (source_dir() / "bindings/python/README.md").read_text()
    return re.findall(r"```python\n(.*?)```", text, flags=re.DOTALL)


class FakeBus:
    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.i2c: list[FakeI2CDevice] = []
        self.uart: list[FakeUARTDevice] = []
        self.uart_sent: dict[int, int] = {}
        self.sleeps: list[float] = []

    def open_i2c(self, bus: int, address: int) -> FakeI2CDevice:
        assert bus == 1
        dev = FakeI2CDevice(address=address)
        with self.lock:
            self.i2c.append(dev)
        return dev

    def open_uart(self, path: str, baud: int = 9600, read_timeout_ms: int = 1000) -> FakeUARTDevice:
        dev = FakeUARTDevice()
        with self.lock:
            self.uart.append(dev)
            self.uart_sent[id(dev)] = 0
        return dev

    def sleep(self, seconds: float) -> None:
        # Seconds, not the milliseconds the send helpers return.
        assert 0 < seconds <= 5, f"sleep({seconds!r})"
        with self.lock:
            self.sleeps.append(seconds)
            for dev in self.i2c:
                if not dev.last_tx:
                    continue
                reply = I2C_REPLIES[dev.address].get(dev.last_tx)
                assert reply is not None, f"no fake reply for {dev.last_tx!r} at {dev.address:#x}"
                dev.set_response(i2c_response(DeviceStatus.SUCCESS, reply))
            for dev in self.uart:
                pending = dev.tx_bytes[self.uart_sent[id(dev)] :]
                for command in pending.split(b"\r")[:-1]:
                    dev.append_response(uart_lines(*UART_REPLIES[command]))
                self.uart_sent[id(dev)] = len(dev.tx_bytes)


def test_readme_has_python_examples():
    assert readme_examples()


# An exception in an example's thread fails the test instead of a warning.
@pytest.mark.filterwarnings("error::pytest.PytestUnhandledThreadExceptionWarning")
@pytest.mark.parametrize("index", range(len(readme_examples())))
def test_readme_example_runs_against_the_fakes(index, monkeypatch, capsys):
    fake = FakeBus()
    monkeypatch.setattr(ezo_driver, "LinuxI2CDevice", fake.open_i2c)
    monkeypatch.setattr(ezo_driver, "LinuxUARTDevice", fake.open_uart)
    monkeypatch.setattr("time.sleep", fake.sleep)

    exec(compile(readme_examples()[index], f"README.md example {index}", "exec"), {})

    assert fake.sleeps, "the example never waits for the circuit"
    assert capsys.readouterr().out
