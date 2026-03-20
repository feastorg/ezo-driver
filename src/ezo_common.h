#ifndef EZO_COMMON_H
#define EZO_COMMON_H

#include "ezo_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

ezo_result_t ezo_common_format_fixed_command(char *buffer,
                                             size_t buffer_len,
                                             const char *prefix,
                                             double value,
                                             uint8_t decimals);

ezo_result_t ezo_common_parse_double(const char *buffer,
                                     size_t buffer_len,
                                     double *value_out);

#ifdef __cplusplus
}
#endif

#endif
