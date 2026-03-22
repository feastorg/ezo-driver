#ifndef EZO_H
#define EZO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  EZO_OK = 0,
  EZO_ERR_INVALID_ARGUMENT,
  EZO_ERR_BUFFER_TOO_SMALL,
  EZO_ERR_TRANSPORT,
  EZO_ERR_PROTOCOL,
  EZO_ERR_PARSE
} ezo_result_t;

typedef enum {
  EZO_COMMAND_GENERIC = 0,
  EZO_COMMAND_READ,
  EZO_COMMAND_READ_WITH_TEMP_COMP,
  EZO_COMMAND_CALIBRATION
} ezo_command_kind_t;

typedef struct {
  uint32_t wait_ms;
} ezo_timing_hint_t;

ezo_result_t ezo_get_timing_hint_for_command_kind(ezo_command_kind_t kind,
                                                  ezo_timing_hint_t *timing_hint);

const char *ezo_result_name(ezo_result_t result);

ezo_result_t ezo_parse_double(const char *buffer, size_t buffer_len, double *value_out);

#ifdef __cplusplus
}
#endif

#endif
