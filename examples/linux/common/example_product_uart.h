#ifndef EZO_EXAMPLE_PRODUCT_UART_H
#define EZO_EXAMPLE_PRODUCT_UART_H

#include "ezo_control.h"

#ifdef __cplusplus
extern "C" {
#endif

ezo_result_t ezo_example_query_info_uart(ezo_uart_device_t *device,
                                         ezo_device_info_t *info_out);

ezo_result_t ezo_example_print_shared_control_uart(ezo_uart_device_t *device,
                                                   ezo_product_id_t product_id);

ezo_result_t ezo_example_print_product_readiness_uart(ezo_uart_device_t *device,
                                                      ezo_product_id_t product_id);

#ifdef __cplusplus
}
#endif

#endif
