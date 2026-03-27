#define EZO_FAKE_I2C_TRANSPORT_MAX_TX ...
#define EZO_FAKE_I2C_TRANSPORT_MAX_RX ...
#define EZO_FAKE_UART_MAX_TX ...
#define EZO_FAKE_UART_MAX_RX ...

typedef struct {
  ezo_result_t callback_result;
  uint8_t expected_address;
  int enforce_expected_address;
  uint8_t response_bytes[EZO_FAKE_I2C_TRANSPORT_MAX_RX];
  size_t response_len;
  size_t call_count;
  uint8_t last_tx_bytes[EZO_FAKE_I2C_TRANSPORT_MAX_TX];
  size_t last_tx_len;
  size_t last_rx_len_requested;
} ezo_fake_i2c_transport_t;

typedef struct {
  ezo_fake_i2c_transport_t transport;
  ezo_i2c_device_t core;
} ezo_py_fake_i2c_device_t;

ezo_result_t ezo_py_fake_i2c_device_init(ezo_py_fake_i2c_device_t *device, uint8_t address);
void ezo_py_fake_i2c_device_set_response(ezo_py_fake_i2c_device_t *device,
                                         const uint8_t *response_bytes,
                                         size_t response_len);
ezo_i2c_device_t *ezo_py_fake_i2c_device_core(ezo_py_fake_i2c_device_t *device);

typedef struct {
  ezo_result_t write_result;
  ezo_result_t read_result;
  ezo_result_t discard_result;
  uint8_t response_bytes[EZO_FAKE_UART_MAX_RX];
  size_t response_len;
  size_t response_offset;
  size_t max_bytes_per_read;
  size_t write_call_count;
  size_t read_call_count;
  size_t discard_call_count;
  uint8_t tx_bytes[EZO_FAKE_UART_MAX_TX];
  size_t tx_len;
} ezo_fake_uart_transport_t;

typedef struct {
  ezo_fake_uart_transport_t transport;
  ezo_uart_device_t core;
} ezo_py_fake_uart_device_t;

ezo_result_t ezo_py_fake_uart_device_init(ezo_py_fake_uart_device_t *device);
void ezo_py_fake_uart_device_set_response(ezo_py_fake_uart_device_t *device,
                                          const uint8_t *response_bytes,
                                          size_t response_len);
void ezo_py_fake_uart_device_append_response(ezo_py_fake_uart_device_t *device,
                                             const uint8_t *response_bytes,
                                             size_t response_len);
ezo_uart_device_t *ezo_py_fake_uart_device_core(ezo_py_fake_uart_device_t *device);
