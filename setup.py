from __future__ import annotations

from setuptools import setup


setup(
    cffi_modules=["bindings/python/build_ffi.py:ffibuilder"],
)
