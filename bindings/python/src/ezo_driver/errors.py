from __future__ import annotations


class EzoError(Exception):
    """Base exception for Python binding failures."""


class EzoArgumentError(EzoError, ValueError):
    """Raised when the core library rejects invalid arguments."""


class EzoTransportError(EzoError):
    """Raised when a transport operation fails."""


class EzoProtocolError(EzoError):
    """Raised when a device response violates the expected protocol."""


class EzoParseError(EzoError):
    """Raised when a response payload cannot be parsed."""


def raise_for_result(result: int, *, message: str | None = None) -> None:
    if result == 0:
        return
    if result == 1:
        raise EzoArgumentError(message or "invalid argument")
    if result == 2:
        raise EzoArgumentError(message or "buffer too small")
    if result == 3:
        raise EzoTransportError(message or "transport failure")
    if result == 4:
        raise EzoProtocolError(message or "protocol failure")
    if result == 5:
        raise EzoParseError(message or "parse failure")
    raise EzoError(message or f"unknown ezo result: {result}")
