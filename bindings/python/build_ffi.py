from __future__ import annotations

from pathlib import Path

from cffi import FFI


THIS_DIR = Path(__file__).resolve().parent
REPO_ROOT = THIS_DIR.parent.parent
SRC_DIR = REPO_ROOT / "src"
LINUX_DIR = REPO_ROOT / "platform" / "linux"
FAKES_DIR = REPO_ROOT / "tests" / "fakes"
CDEF_DIR = THIS_DIR / "cdef"
TESTSUPPORT_DIR = THIS_DIR / "testsupport"

ffibuilder = FFI()
ffibuilder.cdef((CDEF_DIR / "ezo_api.h").read_text(encoding="ascii"))
ffibuilder.cdef((CDEF_DIR / "ezo_test_bridge.h").read_text(encoding="ascii"))

ffibuilder.set_source(
    "ezo_driver._native",
    '\n'.join(
        [
            '#include "ezo.h"',
            '#include "ezo_common.h"',
            '#include "ezo_i2c.h"',
            '#include "ezo_uart.h"',
            '#include "ezo_product.h"',
            '#include "ezo_parse.h"',
            '#include "ezo_schema.h"',
            '#include "ezo_control.h"',
            '#include "ezo_calibration_transfer.h"',
            '#include "ezo_ph.h"',
            '#include "ezo_orp.h"',
            '#include "ezo_rtd.h"',
            '#include "ezo_ec.h"',
            '#include "ezo_do.h"',
            '#include "ezo_hum.h"',
            '#include "ezo_linux_device.h"',
            '#include "tests/fakes/ezo_fake_i2c_transport.h"',
            '#include "tests/fakes/ezo_fake_uart_transport.h"',
            '#include "ezo_test_bridge.h"',
        ]
    ),
    sources=[
        *sorted(str(path) for path in SRC_DIR.glob("*.c")),
        str(LINUX_DIR / "ezo_i2c_linux_i2c.c"),
        str(LINUX_DIR / "ezo_uart_posix_serial.c"),
        str(LINUX_DIR / "ezo_linux_device.c"),
        str(FAKES_DIR / "ezo_fake_i2c_transport.c"),
        str(FAKES_DIR / "ezo_fake_uart_transport.c"),
        str(TESTSUPPORT_DIR / "ezo_test_bridge.c"),
    ],
    include_dirs=[
        str(REPO_ROOT),
        str(SRC_DIR),
        str(LINUX_DIR),
        str(FAKES_DIR),
        str(TESTSUPPORT_DIR),
    ],
    define_macros=[("_GNU_SOURCE", "1")],
)


if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
