#include "ezo_product.h"

#include <assert.h>
#include <string.h>

static void test_parse_device_info_for_initial_products(void) {
  static const struct {
    const char *response;
    ezo_product_id_t expected_product_id;
    const char *expected_code;
    const char *expected_firmware;
  } cases[] = {
      {"?i,pH,2.16", EZO_PRODUCT_PH, "pH", "2.16"},
      {"?i,ORP,1.97", EZO_PRODUCT_ORP, "ORP", "1.97"},
      {"?i,EC,2.16", EZO_PRODUCT_EC, "EC", "2.16"},
      {"?i, D.O., 1.98", EZO_PRODUCT_DO, "D.O.", "1.98"},
      {"?i,RTD,2.01", EZO_PRODUCT_RTD, "RTD", "2.01"},
      {"?i,HUM,1.0", EZO_PRODUCT_HUM, "HUM", "1.0"},
  };
  size_t i = 0;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    ezo_device_info_t info;
    assert(ezo_parse_device_info(cases[i].response, strlen(cases[i].response), &info) == EZO_OK);
    assert(info.product_id == cases[i].expected_product_id);
    assert(strcmp(info.product_code, cases[i].expected_code) == 0);
    assert(strcmp(info.firmware_version, cases[i].expected_firmware) == 0);
  }
}

static void test_parse_device_info_accepts_unrecognized_product_code(void) {
  const char *response = "?i,CO2,1.00";
  ezo_device_info_t info;

  assert(ezo_parse_device_info(response, strlen(response), &info) == EZO_OK);
  assert(info.product_id == EZO_PRODUCT_UNKNOWN);
  assert(strcmp(info.product_code, "CO2") == 0);
  assert(strcmp(info.firmware_version, "1.00") == 0);
}

static void test_parse_device_info_rejects_non_identity_payloads(void) {
  const char *response = "?Status,P,5.038";
  ezo_device_info_t info;

  assert(ezo_parse_device_info(response, strlen(response), &info) == EZO_ERR_PARSE);
}

static void test_product_lookup_accepts_normalized_short_codes(void) {
  ezo_product_id_t product_id = EZO_PRODUCT_UNKNOWN;

  assert(ezo_product_id_from_short_code("pH", strlen("pH"), &product_id) == EZO_OK);
  assert(product_id == EZO_PRODUCT_PH);

  assert(ezo_product_id_from_short_code(" D.O. ", strlen(" D.O. "), &product_id) == EZO_OK);
  assert(product_id == EZO_PRODUCT_DO);

  assert(ezo_product_id_from_short_code("DO", strlen("DO"), &product_id) == EZO_OK);
  assert(product_id == EZO_PRODUCT_DO);

  assert(ezo_product_id_from_short_code("hum", strlen("hum"), &product_id) == EZO_OK);
  assert(product_id == EZO_PRODUCT_HUM);

  assert(ezo_product_id_from_short_code("co2", strlen("co2"), &product_id) == EZO_OK);
  assert(product_id == EZO_PRODUCT_UNKNOWN);
}

static void test_product_metadata_registry_exposes_defaults_and_support_tiers(void) {
  const ezo_product_metadata_t *ph = ezo_product_get_metadata(EZO_PRODUCT_PH);
  const ezo_product_metadata_t *ec = ezo_product_get_metadata(EZO_PRODUCT_EC);
  const ezo_product_metadata_t *hum = ezo_product_get_metadata(EZO_PRODUCT_HUM);
  const ezo_product_metadata_t *unknown = ezo_product_get_metadata(EZO_PRODUCT_UNKNOWN);

  assert(ph != NULL);
  assert(ph->default_transport == EZO_PRODUCT_TRANSPORT_UART);
  assert(ph->default_i2c_address == 99);
  assert(ph->default_continuous_mode == EZO_PRODUCT_DEFAULT_ENABLED);
  assert(ph->default_response_codes == EZO_PRODUCT_DEFAULT_ENABLED);
  assert(ph->default_output_schema == EZO_PRODUCT_OUTPUT_SCHEMA_SCALAR_SINGLE);
  assert(ph->default_output_count == 1);
  assert(ph->support_tier == EZO_PRODUCT_SUPPORT_METADATA);

  assert(ec != NULL);
  assert(ec->default_output_schema == EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED);
  assert(ec->default_output_count == 0);

  assert(hum != NULL);
  assert(hum->default_output_schema == EZO_PRODUCT_OUTPUT_SCHEMA_PRIMARY_ONLY);
  assert(hum->default_output_count == 1);

  assert(unknown == NULL);
  assert(ezo_product_get_support_tier(EZO_PRODUCT_UNKNOWN) == EZO_PRODUCT_SUPPORT_UNKNOWN);
}

static void test_product_lookup_by_short_code_returns_metadata(void) {
  const ezo_product_metadata_t *metadata = NULL;

  metadata = ezo_product_get_metadata_by_short_code("RTD", strlen("RTD"));
  assert(metadata != NULL);
  assert(metadata->product_id == EZO_PRODUCT_RTD);

  metadata = ezo_product_get_metadata_by_short_code(" D.O. ", strlen(" D.O. "));
  assert(metadata != NULL);
  assert(metadata->product_id == EZO_PRODUCT_DO);

  metadata = ezo_product_get_metadata_by_short_code("CO2", strlen("CO2"));
  assert(metadata == NULL);
}

static void test_product_capability_and_family_lookups(void) {
  assert(ezo_product_supports_capability(EZO_PRODUCT_PH,
                                         EZO_PRODUCT_CAP_TEMPERATURE_COMPENSATION) == 1);
  assert(ezo_product_supports_capability(EZO_PRODUCT_ORP,
                                         EZO_PRODUCT_CAP_TEMPERATURE_COMPENSATION) == 0);
  assert(ezo_product_supports_capability(EZO_PRODUCT_DO,
                                         EZO_PRODUCT_CAP_SALINITY_COMPENSATION |
                                             EZO_PRODUCT_CAP_PRESSURE_COMPENSATION) == 1);
  assert(ezo_product_supports_capability(EZO_PRODUCT_RTD,
                                         EZO_PRODUCT_CAP_DATA_LOGGING |
                                             EZO_PRODUCT_CAP_MEMORY_RECALL) == 1);
  assert(ezo_product_supports_capability(EZO_PRODUCT_HUM,
                                         EZO_PRODUCT_CAP_OUTPUT_SELECTION) == 1);

  assert(ezo_product_has_command_family(EZO_PRODUCT_HUM,
                                        EZO_PRODUCT_FAMILY_CALIBRATION) == 1);
  assert(ezo_product_has_command_family(EZO_PRODUCT_HUM,
                                        EZO_PRODUCT_FAMILY_CALIBRATION_TRANSFER) == 0);
  assert(ezo_product_has_command_family(EZO_PRODUCT_EC,
                                        EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL) == 1);
}

static void test_product_timing_lookup_uses_transport_specific_profiles(void) {
  ezo_timing_hint_t hint;

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_PH,
                                     EZO_PRODUCT_TRANSPORT_UART,
                                     EZO_COMMAND_READ,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 900);

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_HUM,
                                     EZO_PRODUCT_TRANSPORT_I2C,
                                     EZO_COMMAND_READ,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 300);

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_DO,
                                     EZO_PRODUCT_TRANSPORT_UART,
                                     EZO_COMMAND_CALIBRATION,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 1300);

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_RTD,
                                     EZO_PRODUCT_TRANSPORT_I2C,
                                     EZO_COMMAND_READ,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 600);

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_ORP,
                                     EZO_PRODUCT_TRANSPORT_UART,
                                     EZO_COMMAND_READ_WITH_TEMP_COMP,
                                     &hint) == EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_product_get_timing_hint(EZO_PRODUCT_UNKNOWN,
                                     EZO_PRODUCT_TRANSPORT_UART,
                                     EZO_COMMAND_READ,
                                     &hint) == EZO_ERR_INVALID_ARGUMENT);
}

int main(void) {
  test_parse_device_info_for_initial_products();
  test_parse_device_info_accepts_unrecognized_product_code();
  test_parse_device_info_rejects_non_identity_payloads();
  test_product_lookup_accepts_normalized_short_codes();
  test_product_metadata_registry_exposes_defaults_and_support_tiers();
  test_product_lookup_by_short_code_returns_metadata();
  test_product_capability_and_family_lookups();
  test_product_timing_lookup_uses_transport_specific_profiles();
  return 0;
}
