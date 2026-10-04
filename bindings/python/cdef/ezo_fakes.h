#define EZO_FAKE_I2C_TRANSPORT_MAX_TX 128
#define EZO_FAKE_I2C_TRANSPORT_MAX_RX 128
#define EZO_FAKE_UART_MAX_TX 256
#define EZO_FAKE_UART_MAX_RX 1024

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

void ezo_fake_i2c_transport_init(ezo_fake_i2c_transport_t *fake);
void ezo_fake_i2c_transport_set_response(ezo_fake_i2c_transport_t *fake,
                                         const uint8_t *response_bytes,
                                         size_t response_len);
const ezo_i2c_transport_t *ezo_fake_i2c_transport_vtable(void);

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

void ezo_fake_uart_transport_init(ezo_fake_uart_transport_t *fake);
void ezo_fake_uart_transport_set_response(ezo_fake_uart_transport_t *fake,
                                          const uint8_t *response_bytes,
                                          size_t response_len);
void ezo_fake_uart_transport_append_response(ezo_fake_uart_transport_t *fake,
                                             const uint8_t *response_bytes,
                                             size_t response_len);
const ezo_uart_transport_t *ezo_fake_uart_transport_vtable(void);
