# cython: language_level=3
from libc.stddef cimport size_t
from libc.stdint cimport uint8_t, uint32_t
from cpython.bytes cimport PyBytes_FromStringAndSize
from .errors import (
    EzoArgumentError,
    EzoError,
    EzoParseError,
    EzoProtocolError,
    EzoTransportError,
)

cdef extern from "ezo.h":
    ctypedef enum ezo_result_t:
        EZO_OK
        EZO_ERR_INVALID_ARGUMENT
        EZO_ERR_BUFFER_TOO_SMALL
        EZO_ERR_TRANSPORT
        EZO_ERR_PROTOCOL
        EZO_ERR_PARSE

    ctypedef enum ezo_command_kind_t:
        EZO_COMMAND_GENERIC
        EZO_COMMAND_READ
        EZO_COMMAND_READ_WITH_TEMP_COMP
        EZO_COMMAND_CALIBRATION

    ctypedef struct ezo_timing_hint_t:
        uint32_t wait_ms

    const char *ezo_result_name(ezo_result_t result)

cdef extern from "fcntl.h":
    int O_RDWR
    int open(const char *pathname, int flags)

cdef extern from "unistd.h":
    int close(int fd)

cdef extern from "ezo_i2c.h":
    ctypedef enum ezo_device_status_t:
        EZO_STATUS_UNKNOWN
        EZO_STATUS_SUCCESS
        EZO_STATUS_FAIL
        EZO_STATUS_NOT_READY
        EZO_STATUS_NO_DATA

    ctypedef struct ezo_i2c_transport_t:
        pass

    ctypedef struct ezo_i2c_device_t:
        uint8_t address
        const ezo_i2c_transport_t *transport
        void *transport_context
        uint8_t last_device_status

    ezo_result_t ezo_device_init(ezo_i2c_device_t *device,
                                 uint8_t address,
                                 const ezo_i2c_transport_t *transport,
                                 void *transport_context)

    ezo_result_t ezo_send_command(ezo_i2c_device_t *device,
                                  const char *command,
                                  ezo_command_kind_t kind,
                                  ezo_timing_hint_t *timing_hint)

    ezo_result_t ezo_send_command_with_float(ezo_i2c_device_t *device,
                                             const char *prefix,
                                             double value,
                                             uint8_t decimals,
                                             ezo_command_kind_t kind,
                                             ezo_timing_hint_t *timing_hint)

    ezo_result_t ezo_send_read(ezo_i2c_device_t *device, ezo_timing_hint_t *timing_hint)
    ezo_result_t ezo_send_read_with_temp_comp(ezo_i2c_device_t *device,
                                              double temperature_c,
                                              uint8_t decimals,
                                              ezo_timing_hint_t *timing_hint)

    ezo_result_t ezo_read_response(ezo_i2c_device_t *device,
                                   char *buffer,
                                   size_t buffer_len,
                                   size_t *response_len,
                                   ezo_device_status_t *device_status)

cdef extern from "ezo_i2c_linux_i2c.h":
    ctypedef struct ezo_linux_i2c_context_t:
        int fd

    ezo_result_t ezo_linux_i2c_context_init(ezo_linux_i2c_context_t *context, int fd)
    const ezo_i2c_transport_t *ezo_linux_i2c_transport()

cdef extern from "ezo_uart.h":
    ctypedef enum ezo_uart_response_kind_t:
        EZO_UART_RESPONSE_UNKNOWN
        EZO_UART_RESPONSE_DATA
        EZO_UART_RESPONSE_OK
        EZO_UART_RESPONSE_ERROR
        EZO_UART_RESPONSE_OVER_VOLTAGE
        EZO_UART_RESPONSE_UNDER_VOLTAGE
        EZO_UART_RESPONSE_RESET
        EZO_UART_RESPONSE_READY
        EZO_UART_RESPONSE_SLEEP
        EZO_UART_RESPONSE_WAKE
        EZO_UART_RESPONSE_DONE

    ctypedef struct ezo_uart_transport_t:
        pass

    ctypedef struct ezo_uart_device_t:
        const ezo_uart_transport_t *transport
        void *transport_context
        uint8_t last_response_kind

    ezo_result_t ezo_uart_device_init(ezo_uart_device_t *device,
                                      const ezo_uart_transport_t *transport,
                                      void *transport_context)

    ezo_result_t ezo_uart_send_command(ezo_uart_device_t *device,
                                       const char *command,
                                       ezo_command_kind_t kind,
                                       ezo_timing_hint_t *timing_hint)

    ezo_result_t ezo_uart_send_command_with_float(ezo_uart_device_t *device,
                                                  const char *prefix,
                                                  double value,
                                                  uint8_t decimals,
                                                  ezo_command_kind_t kind,
                                                  ezo_timing_hint_t *timing_hint)

    ezo_result_t ezo_uart_send_read(ezo_uart_device_t *device, ezo_timing_hint_t *timing_hint)
    ezo_result_t ezo_uart_send_read_with_temp_comp(ezo_uart_device_t *device,
                                                   double temperature_c,
                                                   uint8_t decimals,
                                                   ezo_timing_hint_t *timing_hint)

    ezo_result_t ezo_uart_read_line(ezo_uart_device_t *device,
                                    char *buffer,
                                    size_t buffer_len,
                                    size_t *response_len,
                                    ezo_uart_response_kind_t *response_kind)

    ezo_result_t ezo_uart_read_terminal_response(ezo_uart_device_t *device,
                                                 ezo_uart_response_kind_t *response_kind)

    ezo_result_t ezo_uart_read_ok(ezo_uart_device_t *device)
    ezo_result_t ezo_uart_discard_input(ezo_uart_device_t *device)

cdef extern from "ezo_uart_posix_serial.h":
    ctypedef enum ezo_uart_posix_baud_t:
        EZO_UART_POSIX_BAUD_300
        EZO_UART_POSIX_BAUD_1200
        EZO_UART_POSIX_BAUD_2400
        EZO_UART_POSIX_BAUD_9600
        EZO_UART_POSIX_BAUD_19200
        EZO_UART_POSIX_BAUD_38400
        EZO_UART_POSIX_BAUD_57600
        EZO_UART_POSIX_BAUD_115200

    ctypedef struct ezo_uart_posix_serial_t:
        int fd
        uint32_t read_timeout_ms
        int owns_fd
        int has_saved_termios

    ezo_result_t ezo_uart_posix_serial_open(ezo_uart_posix_serial_t *serial,
                                            const char *path,
                                            ezo_uart_posix_baud_t baud,
                                            uint32_t read_timeout_ms)

    void ezo_uart_posix_serial_close(ezo_uart_posix_serial_t *serial)
    const ezo_uart_transport_t *ezo_uart_posix_serial_transport()


cdef void _check_result(ezo_result_t result):
    if result != EZO_OK:
        name = ezo_result_name(result)
        message = f"EZO error {int(result)}"
        if name != NULL:
            message = (<bytes>name).decode("ascii", errors="replace")
        if result == EZO_ERR_INVALID_ARGUMENT:
            raise EzoArgumentError(message)
        if result == EZO_ERR_TRANSPORT:
            raise EzoTransportError(message)
        if result == EZO_ERR_PROTOCOL:
            raise EzoProtocolError(message)
        if result == EZO_ERR_PARSE:
            raise EzoParseError(message)
        raise EzoError(message)


cdef ezo_command_kind_t _cmd_kind(object kind):
    if isinstance(kind, int):
        if kind < 0 or kind > 3:
            raise ValueError("kind out of range")
        return <ezo_command_kind_t>kind

    text = str(kind).strip().lower()
    if text in ("generic", "command", "ezo_command_generic"):
        return EZO_COMMAND_GENERIC
    if text in ("read", "ezo_command_read"):
        return EZO_COMMAND_READ
    if text in ("read_with_temp", "read_with_temp_comp", "temp", "ezo_command_read_with_temp_comp"):
        return EZO_COMMAND_READ_WITH_TEMP_COMP
    if text in ("calibration", "cal", "ezo_command_calibration"):
        return EZO_COMMAND_CALIBRATION

    raise ValueError(f"unknown command kind: {kind!r}")


cdef ezo_uart_posix_baud_t _map_baud(int baud):
    if baud == 300:
        return EZO_UART_POSIX_BAUD_300
    if baud == 1200:
        return EZO_UART_POSIX_BAUD_1200
    if baud == 2400:
        return EZO_UART_POSIX_BAUD_2400
    if baud == 9600:
        return EZO_UART_POSIX_BAUD_9600
    if baud == 19200:
        return EZO_UART_POSIX_BAUD_19200
    if baud == 38400:
        return EZO_UART_POSIX_BAUD_38400
    if baud == 57600:
        return EZO_UART_POSIX_BAUD_57600
    if baud == 115200:
        return EZO_UART_POSIX_BAUD_115200
    raise ValueError("unsupported baud; expected one of 300..115200 supported by adapter")


cdef class I2CDeviceLinux:
    cdef ezo_i2c_device_t _device
    cdef ezo_linux_i2c_context_t _context
    cdef int _fd

    def __cinit__(self, int bus, int address):
        cdef bytes path
        cdef ezo_result_t result

        if bus < 0:
            raise ValueError("bus must be >= 0")
        if address < 0 or address > 0x7F:
            raise ValueError("I2C address must be in [0, 127]")

        self._fd = -1
        path = f"/dev/i2c-{bus}".encode("ascii")
        self._fd = open(path, O_RDWR)
        if self._fd < 0:
            raise OSError(f"failed to open {path.decode('ascii')}")

        result = ezo_linux_i2c_context_init(&self._context, self._fd)
        _check_result(result)

        result = ezo_device_init(&self._device,
                                 <uint8_t>address,
                                 ezo_linux_i2c_transport(),
                                 <void *>&self._context)
        _check_result(result)

    def close(self):
        if self._fd >= 0:
            close(self._fd)
            self._fd = -1

    def __dealloc__(self):
        if self._fd >= 0:
            close(self._fd)
            self._fd = -1

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()
        return False

    @property
    def address(self):
        return int(self._device.address)

    def send_command(self, str command, kind="generic"):
        cdef ezo_timing_hint_t hint
        cdef bytes b = command.encode("ascii")
        _check_result(ezo_send_command(&self._device, b, _cmd_kind(kind), &hint))
        return int(hint.wait_ms)

    def send_command_with_float(self, str prefix, double value, int decimals=2, kind="generic"):
        cdef ezo_timing_hint_t hint
        cdef bytes p = prefix.encode("ascii")
        if decimals < 0 or decimals > 6:
            raise ValueError("decimals must be in [0, 6]")
        _check_result(ezo_send_command_with_float(&self._device,
                                                  p,
                                                  value,
                                                  <uint8_t>decimals,
                                                  _cmd_kind(kind),
                                                  &hint))
        return int(hint.wait_ms)

    def send_read(self):
        cdef ezo_timing_hint_t hint
        _check_result(ezo_send_read(&self._device, &hint))
        return int(hint.wait_ms)

    def send_read_with_temp_comp(self, double temperature_c, int decimals=2):
        cdef ezo_timing_hint_t hint
        _check_result(ezo_send_read_with_temp_comp(&self._device,
                                                   temperature_c,
                                                   <uint8_t>decimals,
                                                   &hint))
        return int(hint.wait_ms)

    def read_response(self):
        cdef char buffer[256]
        cdef size_t response_len = 0
        cdef ezo_device_status_t status = EZO_STATUS_UNKNOWN
        cdef object text
        _check_result(ezo_read_response(&self._device,
                                        buffer,
                                        sizeof(buffer),
                                        &response_len,
                                        &status))
        text = (<bytes>PyBytes_FromStringAndSize(buffer, response_len)).decode("ascii", errors="replace")
        return int(status), text


cdef class UARTDeviceLinux:
    cdef ezo_uart_posix_serial_t _serial
    cdef ezo_uart_device_t _device
    cdef bint _open

    def __cinit__(self, path: str, int baud=9600, int read_timeout_ms=1000):
        cdef ezo_result_t result
        cdef bytes bpath = path.encode("utf-8")
        if read_timeout_ms <= 0:
            raise ValueError("read_timeout_ms must be > 0")

        self._open = False
        result = ezo_uart_posix_serial_open(&self._serial,
                                            bpath,
                                            _map_baud(baud),
                                            <uint32_t>read_timeout_ms)
        _check_result(result)
        self._open = True

        result = ezo_uart_device_init(&self._device,
                                      ezo_uart_posix_serial_transport(),
                                      <void *>&self._serial)
        _check_result(result)

    def close(self):
        if self._open:
            ezo_uart_posix_serial_close(&self._serial)
            self._open = False

    def __dealloc__(self):
        if self._open:
            ezo_uart_posix_serial_close(&self._serial)
            self._open = False

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()
        return False

    def send_command(self, str command, kind="generic"):
        cdef ezo_timing_hint_t hint
        cdef bytes b = command.encode("ascii")
        _check_result(ezo_uart_send_command(&self._device, b, _cmd_kind(kind), &hint))
        return int(hint.wait_ms)

    def send_command_with_float(self, str prefix, double value, int decimals=2, kind="generic"):
        cdef ezo_timing_hint_t hint
        cdef bytes p = prefix.encode("ascii")
        _check_result(ezo_uart_send_command_with_float(&self._device,
                                                       p,
                                                       value,
                                                       <uint8_t>decimals,
                                                       _cmd_kind(kind),
                                                       &hint))
        return int(hint.wait_ms)

    def send_read(self):
        cdef ezo_timing_hint_t hint
        _check_result(ezo_uart_send_read(&self._device, &hint))
        return int(hint.wait_ms)

    def send_read_with_temp_comp(self, double temperature_c, int decimals=2):
        cdef ezo_timing_hint_t hint
        _check_result(ezo_uart_send_read_with_temp_comp(&self._device,
                                                        temperature_c,
                                                        <uint8_t>decimals,
                                                        &hint))
        return int(hint.wait_ms)

    def read_line(self):
        cdef char buffer[256]
        cdef size_t response_len = 0
        cdef ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN
        cdef object text
        _check_result(ezo_uart_read_line(&self._device,
                                         buffer,
                                         sizeof(buffer),
                                         &response_len,
                                         &kind))
        text = (<bytes>PyBytes_FromStringAndSize(buffer, response_len)).decode("ascii", errors="replace")
        return int(kind), text

    def read_terminal_response(self):
        cdef ezo_uart_response_kind_t kind = EZO_UART_RESPONSE_UNKNOWN
        _check_result(ezo_uart_read_terminal_response(&self._device, &kind))
        return int(kind)

    def read_ok(self):
        _check_result(ezo_uart_read_ok(&self._device))

    def discard_input(self):
        _check_result(ezo_uart_discard_input(&self._device))


__all__ = [
    "I2CDeviceLinux",
    "UARTDeviceLinux",
]
