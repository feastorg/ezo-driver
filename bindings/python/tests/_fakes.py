"""Build and load the fake transports as a test-only cffi module.

The shipped ``ezo_driver._native`` module carries the core and the Linux
transports only. The tests compile ``tests/fakes/`` into a separate module
here, in a temporary directory, and share C types with ``_native`` through
``FFI.include`` so a fake transport can be wired into a real device.

The C sources are found in the repo checkout (or unpacked sdist) that holds
this file, or in the directory named by ``EZO_DRIVER_SOURCE_DIR`` when the
tests run from somewhere else.
"""

from __future__ import annotations

import importlib.util
import os
import tempfile
from pathlib import Path

from cffi import FFI


SOURCE_DIR_ENV = "EZO_DRIVER_SOURCE_DIR"
MODULE_NAME = "_ezo_fakes"
REQUIRED_FILES = (
    "bindings/python/build_ffi.py",
    "bindings/python/cdef/ezo_fakes.h",
    "tests/fakes/ezo_fake_i2c_transport.c",
    "tests/fakes/ezo_fake_uart_transport.c",
)


def source_dir() -> Path:
    configured = os.environ.get(SOURCE_DIR_ENV)
    if configured:
        root = Path(configured).resolve()
        where = f"{SOURCE_DIR_ENV}={configured}"
    else:
        root = Path(__file__).resolve().parents[3]
        where = f"{root} (set {SOURCE_DIR_ENV} to the ezo-driver checkout)"

    missing = [name for name in REQUIRED_FILES if not (root / name).is_file()]
    if missing:
        raise RuntimeError(f"ezo-driver sources not found under {where}: missing {', '.join(missing)}")
    return root


def _load_from_path(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _build(root: Path):
    native = _load_from_path("ezo_build_ffi", root / "bindings" / "python" / "build_ffi.py")

    builder = FFI()
    builder.include(native.ffibuilder)
    builder.cdef((root / "bindings" / "python" / "cdef" / "ezo_fakes.h").read_text(encoding="ascii"))
    headers = [*native.HEADERS, "tests/fakes/ezo_fake_i2c_transport.h", "tests/fakes/ezo_fake_uart_transport.h"]
    builder.set_source(
        MODULE_NAME,
        '\n'.join(f'#include "{header}"' for header in headers),
        sources=[
            str(root / "tests" / "fakes" / "ezo_fake_i2c_transport.c"),
            str(root / "tests" / "fakes" / "ezo_fake_uart_transport.c"),
        ],
        include_dirs=native.INCLUDE_DIRS,
    )

    build_dir = tempfile.TemporaryDirectory(prefix="ezo-driver-fakes-")
    module = _load_from_path(MODULE_NAME, Path(builder.compile(tmpdir=build_dir.name)))
    module.build_dir = build_dir  # removed when the interpreter exits
    return module


_module = _build(source_dir())
ffi = _module.ffi
lib = _module.lib

__all__ = ["ffi", "lib", "source_dir"]
