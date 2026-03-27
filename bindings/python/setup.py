from __future__ import annotations

from setuptools import setup


setup(
    cffi_modules=["build_ffi.py:ffibuilder"],
)
