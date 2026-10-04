from __future__ import annotations

from pathlib import Path

from cffi import FFI


THIS_DIR = Path(__file__).resolve().parent
REPO_ROOT = THIS_DIR.parent.parent
SRC_DIR = REPO_ROOT / "src"
LINUX_DIR = REPO_ROOT / "platform" / "linux"
CDEF_DIR = THIS_DIR / "cdef"

# The headers behind the cdef. The test-only fakes module (bindings/python/
# tests/_fakes.py) includes this ffibuilder and compiles against the same list.
HEADERS = [
    "ezo.h",
    "ezo_common.h",
    "ezo_i2c.h",
    "ezo_uart.h",
    "ezo_product.h",
    "ezo_parse.h",
    "ezo_schema.h",
    "ezo_control.h",
    "ezo_calibration_transfer.h",
    "ezo_ph.h",
    "ezo_orp.h",
    "ezo_rtd.h",
    "ezo_ec.h",
    "ezo_do.h",
    "ezo_hum.h",
    "ezo_linux_device.h",
]
INCLUDE_DIRS = [str(REPO_ROOT), str(SRC_DIR), str(LINUX_DIR)]

ffibuilder = FFI()
ffibuilder.cdef((CDEF_DIR / "ezo_api.h").read_text(encoding="ascii"))

ffibuilder.set_source(
    "ezo_driver._native",
    '\n'.join(f'#include "{header}"' for header in HEADERS),
    sources=[
        *sorted(str(path) for path in SRC_DIR.glob("*.c")),
        str(LINUX_DIR / "ezo_i2c_linux_i2c.c"),
        str(LINUX_DIR / "ezo_uart_posix_serial.c"),
        str(LINUX_DIR / "ezo_linux_device.c"),
    ],
    include_dirs=INCLUDE_DIRS,
    define_macros=[("_GNU_SOURCE", "1")],
)


if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
