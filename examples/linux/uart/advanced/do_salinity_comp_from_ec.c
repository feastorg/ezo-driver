/*
Purpose: inspect an EC-driven D.O. salinity-compensation chain and optionally apply the source salinity.
Defaults: EC on /dev/ttyUSB0, D.O. on /dev/ttyUSB1, both at 9600 baud.
Assumptions: both devices are reachable over UART, response-code mode can be bootstrapped, and EC salinity output is enabled before apply.
Next: read do_workflow.c for single-device D.O. compensation state and output-setting flows.
*/

#include "example_base.h"
#include "example_uart.h"

#include "ezo_do.h"
#include "ezo_ec.h"

#include <stdio.h>

static const char *salinity_unit_name(ezo_do_salinity_unit_t unit) {
  return unit == EZO_DO_SALINITY_UNIT_PPT ? "ppt" : "us_cm";
}

static void print_ec_reading(const char *prefix, const ezo_ec_reading_t *reading) {
  if ((reading->present_mask & EZO_EC_OUTPUT_CONDUCTIVITY) != 0U) {
    printf("%sconductivity_us_cm=%.3f\n", prefix, reading->conductivity_us_cm);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_TOTAL_DISSOLVED_SOLIDS) != 0U) {
    printf("%stotal_dissolved_solids_ppm=%.3f\n", prefix, reading->total_dissolved_solids_ppm);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SALINITY) != 0U) {
    printf("%ssalinity_ppt=%.3f\n", prefix, reading->salinity_ppt);
  }
  if ((reading->present_mask & EZO_EC_OUTPUT_SPECIFIC_GRAVITY) != 0U) {
    printf("%sspecific_gravity=%.3f\n", prefix, reading->specific_gravity);
  }
}

static void print_do_reading(const char *prefix, const ezo_do_reading_t *reading) {
  if ((reading->present_mask & EZO_DO_OUTPUT_MG_L) != 0U) {
    printf("%smilligrams_per_liter=%.3f\n", prefix, reading->milligrams_per_liter);
  }
  if ((reading->present_mask & EZO_DO_OUTPUT_PERCENT_SATURATION) != 0U) {
    printf("%spercent_saturation=%.3f\n", prefix, reading->percent_saturation);
  }
}

int main(int argc, char **argv) {
  const char *ec_path = EZO_EXAMPLE_UART_DEFAULT_PATH;
  const char *do_path = "/dev/ttyUSB1";
  ezo_uart_posix_baud_t ec_baud = EZO_UART_POSIX_BAUD_9600;
  ezo_uart_posix_baud_t do_baud = EZO_UART_POSIX_BAUD_9600;
  uint32_t ec_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  uint32_t do_baud_rate = EZO_EXAMPLE_UART_DEFAULT_BAUD_RATE;
  ezo_example_uart_session_t ec_session;
  ezo_example_uart_session_t do_session;
  ezo_timing_hint_t hint;
  ezo_ec_output_config_t ec_output_config;
  ezo_ec_reading_t ec_reading;
  ezo_do_output_config_t do_output_config;
  ezo_do_salinity_compensation_t do_salinity;
  ezo_do_reading_t do_reading;
  double source_salinity_ppt = 0.0;
  int source_salinity_available = 0;
  ezo_result_t result = EZO_OK;
  int index = 1;
  int apply_requested = 0;

  apply_requested = ezo_example_has_flag(argc, argv, index, "--apply");

  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--ec-path=");
    if (value != NULL) {
      ec_path = value;
    }
  }
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--do-path=");
    if (value != NULL) {
      do_path = value;
    }
  }
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--ec-baud=");
    if (value != NULL && !ezo_example_parse_baud_arg(value, &ec_baud, &ec_baud_rate)) {
      fprintf(stderr, "invalid --ec-baud value\n");
      return 1;
    }
  }
  {
    const char *value = ezo_example_find_option_value(argc, argv, index, "--do-baud=");
    if (value != NULL && !ezo_example_parse_baud_arg(value, &do_baud, &do_baud_rate)) {
      fprintf(stderr, "invalid --do-baud value\n");
      return 1;
    }
  }

  result = ezo_example_open_uart(ec_path, ec_baud, &ec_session);
  if (result != EZO_OK) {
    return ezo_example_print_error("open_ec_uart", result);
  }

  result = ezo_example_open_uart(do_path, do_baud, &do_session);
  if (result != EZO_OK) {
    ezo_example_close_uart(&ec_session);
    return ezo_example_print_error("open_do_uart", result);
  }

  result =
      ezo_example_uart_bootstrap_response_codes(&ec_session.device, EZO_PRODUCT_EC, NULL, NULL);
  if (result == EZO_OK) {
    result =
        ezo_example_uart_bootstrap_response_codes(&do_session.device, EZO_PRODUCT_DO, NULL, NULL);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_output_query_uart(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_output_config_uart(&ec_session.device, &ec_output_config);
  }
  if (result == EZO_OK) {
    result = ezo_ec_send_read_uart(&ec_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_ec_read_response_uart(&ec_session.device, ec_output_config.enabled_mask, &ec_reading);
  }
  if (result == EZO_OK) {
    source_salinity_available = (ec_reading.present_mask & EZO_EC_OUTPUT_SALINITY) != 0U;
    if (source_salinity_available) {
      source_salinity_ppt = ec_reading.salinity_ppt;
    }
  }
  if (result == EZO_OK) {
    result = ezo_do_send_output_query_uart(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_output_config_uart(&do_session.device, &do_output_config);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_salinity_query_uart(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_salinity_uart(&do_session.device, &do_salinity);
  }
  if (result == EZO_OK) {
    result = ezo_do_send_read_uart(&do_session.device, &hint);
  }
  if (result == EZO_OK) {
    ezo_example_wait_hint(&hint);
    result = ezo_do_read_response_uart(&do_session.device, do_output_config.enabled_mask, &do_reading);
  }

  if (result != EZO_OK) {
    ezo_example_close_uart(&do_session);
    ezo_example_close_uart(&ec_session);
    return ezo_example_print_error("inspect_chain_state", result);
  }

  printf("transport=uart\n");
  printf("ec_path=%s\n", ec_path);
  printf("ec_baud_rate=%u\n", (unsigned)ec_baud_rate);
  printf("do_path=%s\n", do_path);
  printf("do_baud_rate=%u\n", (unsigned)do_baud_rate);
  printf("source_output_mask=%u\n", (unsigned)ec_output_config.enabled_mask);
  print_ec_reading("source_", &ec_reading);
  printf("source_salinity_available=%d\n", source_salinity_available);
  if (source_salinity_available) {
    printf("source_salinity_ppt=%.3f\n", source_salinity_ppt);
  }
  printf("target_output_mask=%u\n", (unsigned)do_output_config.enabled_mask);
  printf("current_target_salinity_value=%.3f\n", do_salinity.value);
  printf("current_target_salinity_unit=%s\n", salinity_unit_name(do_salinity.unit));
  print_do_reading("current_target_", &do_reading);
  printf("apply_requested=%d\n", apply_requested);

  if (!source_salinity_available) {
    printf("apply_blocked_reason=ec_salinity_output_disabled\n");
  } else {
    printf("planned_salinity_ppt=%.3f\n", source_salinity_ppt);
  }

  if (apply_requested) {
    if (!source_salinity_available) {
      fprintf(stderr, "EC salinity output must be enabled before --apply\n");
      ezo_example_close_uart(&do_session);
      ezo_example_close_uart(&ec_session);
      return 1;
    }

    result = ezo_do_send_salinity_set_uart(&do_session.device,
                                           source_salinity_ppt,
                                           EZO_DO_SALINITY_UNIT_PPT,
                                           3,
                                           &hint);
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_uart_read_ok(&do_session.device);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_salinity_query_uart(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_salinity_uart(&do_session.device, &do_salinity);
    }
    if (result == EZO_OK) {
      result = ezo_do_send_read_uart(&do_session.device, &hint);
    }
    if (result == EZO_OK) {
      ezo_example_wait_hint(&hint);
      result = ezo_do_read_response_uart(&do_session.device, do_output_config.enabled_mask, &do_reading);
    }
    if (result != EZO_OK) {
      ezo_example_close_uart(&do_session);
      ezo_example_close_uart(&ec_session);
      return ezo_example_print_error("apply_salinity_compensation", result);
    }

    printf("post_target_salinity_value=%.3f\n", do_salinity.value);
    printf("post_target_salinity_unit=%s\n", salinity_unit_name(do_salinity.unit));
    print_do_reading("post_target_", &do_reading);
  }

  ezo_example_close_uart(&do_session);
  ezo_example_close_uart(&ec_session);
  return 0;
}
