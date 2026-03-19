#ifndef EZO_I2C_EZO_I2C_HPP
#define EZO_I2C_EZO_I2C_HPP

#include "ezo_i2c/ezo_i2c.h"

#include <cstddef>
#include <cstdint>

namespace ezo_i2c {

typedef ezo_result_t Result;
typedef ezo_device_status_t DeviceStatus;
typedef ezo_command_kind_t CommandKind;
typedef ezo_timing_hint_t TimingHint;
typedef ezo_i2c_transport_t Transport;

class Device {
public:
  Device() : device_() {}

  Result init(uint8_t address, const Transport *transport, void *transport_context) {
    return ezo_device_init(&device_, address, transport, transport_context);
  }

  void set_address(uint8_t address) {
    ezo_device_set_address(&device_, address);
  }

  uint8_t address() const {
    return ezo_device_get_address(&device_);
  }

  DeviceStatus last_status() const {
    return ezo_device_get_last_status(&device_);
  }

  Result send_command(const char *command,
                      CommandKind kind,
                      TimingHint *timing_hint = NULL) {
    return ezo_send_command(&device_, command, kind, timing_hint);
  }

  Result send_command_with_float(const char *prefix,
                                 double value,
                                 uint8_t decimals,
                                 CommandKind kind,
                                 TimingHint *timing_hint = NULL) {
    return ezo_send_command_with_float(&device_,
                                       prefix,
                                       value,
                                       decimals,
                                       kind,
                                       timing_hint);
  }

  Result send_read(TimingHint *timing_hint = NULL) {
    return ezo_send_read(&device_, timing_hint);
  }

  Result send_read_with_temp_comp(double temperature_c,
                                  uint8_t decimals,
                                  TimingHint *timing_hint = NULL) {
    return ezo_send_read_with_temp_comp(&device_, temperature_c, decimals, timing_hint);
  }

  Result read_response(char *buffer,
                       std::size_t buffer_len,
                       std::size_t *response_len,
                       DeviceStatus *device_status) {
    return ezo_read_response(&device_, buffer, buffer_len, response_len, device_status);
  }

  Result parse_double(const char *buffer, std::size_t buffer_len, double *value_out) const {
    return ezo_parse_double(buffer, buffer_len, value_out);
  }

  ezo_i2c_device_t *native_handle() {
    return &device_;
  }

  const ezo_i2c_device_t *native_handle() const {
    return &device_;
  }

private:
  ezo_i2c_device_t device_;
};

inline Result get_timing_hint_for_command_kind(CommandKind kind, TimingHint *timing_hint) {
  return ezo_get_timing_hint_for_command_kind(kind, timing_hint);
}

inline Result parse_double(const char *buffer, std::size_t buffer_len, double *value_out) {
  return ezo_parse_double(buffer, buffer_len, value_out);
}

}  // namespace ezo_i2c

#endif
