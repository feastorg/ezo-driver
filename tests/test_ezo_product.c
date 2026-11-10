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
  const char *response = "?i,XYZ,1.00";
  ezo_device_info_t info;

  assert(ezo_parse_device_info(response, strlen(response), &info) == EZO_OK);
  assert(info.product_id == EZO_PRODUCT_UNKNOWN);
  assert(strcmp(info.product_code, "XYZ") == 0);
  assert(strcmp(info.firmware_version, "1.00") == 0);
}

static void test_parse_device_info_accepts_observed_uppercase_identity_variant(void) {
  const char *response = "?I,DO,2.16";
  ezo_device_info_t info;

  assert(ezo_parse_device_info(response, strlen(response), &info) == EZO_OK);
  assert(info.product_id == EZO_PRODUCT_DO);
  assert(strcmp(info.product_code, "DO") == 0);
  assert(strcmp(info.firmware_version, "2.16") == 0);
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
  assert(product_id == EZO_PRODUCT_CO2);

  assert(ezo_product_id_from_short_code("xyz", strlen("xyz"), &product_id) == EZO_OK);
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
  assert(ph->support_tier == EZO_PRODUCT_SUPPORT_FULL);

  assert(ec != NULL);
  assert(ec->support_tier == EZO_PRODUCT_SUPPORT_FULL);
  assert(ec->default_output_schema == EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED);
  assert(ec->default_output_count == 0);

  assert(ezo_product_get_support_tier(EZO_PRODUCT_DO) == EZO_PRODUCT_SUPPORT_FULL);

  assert(hum != NULL);
  assert(hum->support_tier == EZO_PRODUCT_SUPPORT_FULL);
  assert(hum->default_output_schema == EZO_PRODUCT_OUTPUT_SCHEMA_PRIMARY_ONLY);
  assert(hum->default_output_count == 1);

  assert(unknown == NULL);
  assert(ezo_product_get_support_tier(EZO_PRODUCT_ORP) == EZO_PRODUCT_SUPPORT_FULL);
  assert(ezo_product_get_support_tier(EZO_PRODUCT_RTD) == EZO_PRODUCT_SUPPORT_FULL);
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

  metadata = ezo_product_get_metadata_by_short_code("XYZ", strlen("XYZ"));
  assert(metadata == NULL);
}

static void test_parse_device_info_identifies_metadata_tier_products(void) {
  static const struct {
    const char *response;
    ezo_product_id_t expected_product_id;
    const char *expected_firmware;
  } cases[] = {
      {"?i,CO2,1.0", EZO_PRODUCT_CO2, "1.0"},    {"?i,O2,1.00", EZO_PRODUCT_O2, "1.00"},
      {"?i,RGB,2.1", EZO_PRODUCT_RGB, "2.1"},    {"?i,PRS, 1.0", EZO_PRODUCT_PRS, "1.0"},
      {"?i,FLO,2.00", EZO_PRODUCT_FLO, "2.00"},  {"?i,PMP, 1.1", EZO_PRODUCT_PMP, "1.1"},
      {"?i,PMPL, 1.1", EZO_PRODUCT_PMP_L, "1.1"},
  };
  size_t i = 0;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    ezo_device_info_t info;
    assert(ezo_parse_device_info(cases[i].response, strlen(cases[i].response), &info) == EZO_OK);
    assert(info.product_id == cases[i].expected_product_id);
    assert(strcmp(info.firmware_version, cases[i].expected_firmware) == 0);
  }
}

static void assert_hint(ezo_product_id_t product_id,
                        ezo_command_kind_t kind,
                        uint32_t expected_ms) {
  static const ezo_product_transport_t transports[] = {EZO_PRODUCT_TRANSPORT_UART,
                                                       EZO_PRODUCT_TRANSPORT_I2C};
  size_t i = 0;

  for (i = 0; i < sizeof(transports) / sizeof(transports[0]); ++i) {
    ezo_timing_hint_t hint;
    ezo_result_t result = ezo_product_get_timing_hint(product_id, transports[i], kind, &hint);
    if (expected_ms == 0U) {
      assert(result == EZO_ERR_INVALID_ARGUMENT);
    } else {
      assert(result == EZO_OK);
      assert(hint.wait_ms == expected_ms);
    }
  }
}

static void test_metadata_tier_products_record_their_facts(void) {
  static const struct {
    ezo_product_id_t product_id;
    const char *short_code;
    uint8_t address;
    ezo_product_output_schema_t schema;
    uint8_t output_count;
    uint32_t read_ms;
    uint32_t read_with_temp_comp_ms;
    uint32_t calibration_ms;
  } cases[] = {
      {EZO_PRODUCT_CO2, "CO2", 105, EZO_PRODUCT_OUTPUT_SCHEMA_PRIMARY_ONLY, 1, 900, 0, 900},
      {EZO_PRODUCT_O2, "O2", 108, EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED, 0, 900, 900, 1300},
      {EZO_PRODUCT_RGB, "RGB", 112, EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED, 0, 300, 0, 300},
      {EZO_PRODUCT_PRS, "PRS", 106, EZO_PRODUCT_OUTPUT_SCHEMA_SCALAR_SINGLE, 1, 900, 0, 900},
      {EZO_PRODUCT_FLO, "FLO", 104, EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED, 0, 300, 0, 0},
      {EZO_PRODUCT_PMP, "PMP", 103, EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED, 0, 600, 0, 300},
      {EZO_PRODUCT_PMP_L, "PMPL", 109, EZO_PRODUCT_OUTPUT_SCHEMA_QUERY_REQUIRED, 0, 600, 0, 300},
  };
  size_t i = 0;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    const ezo_product_metadata_t *metadata = ezo_product_get_metadata(cases[i].product_id);
    ezo_product_id_t product_id = EZO_PRODUCT_UNKNOWN;

    assert(metadata != NULL);
    assert(strcmp(metadata->vendor_short_code, cases[i].short_code) == 0);
    assert(metadata->support_tier == (cases[i].product_id == EZO_PRODUCT_O2 ||
                                              cases[i].product_id == EZO_PRODUCT_CO2
                                          ? EZO_PRODUCT_SUPPORT_TYPED_READ
                                          : EZO_PRODUCT_SUPPORT_METADATA));
    assert(metadata->default_transport == EZO_PRODUCT_TRANSPORT_UART);
    assert(metadata->default_i2c_address == cases[i].address);
    assert(metadata->default_continuous_mode == EZO_PRODUCT_DEFAULT_ENABLED);
    assert(metadata->default_response_codes == EZO_PRODUCT_DEFAULT_ENABLED);
    assert(metadata->default_output_schema == cases[i].schema);
    assert(metadata->default_output_count == cases[i].output_count);
    assert((metadata->capability_flags & EZO_PRODUCT_CAP_CALIBRATION_TRANSFER) == 0U);

    assert(ezo_product_id_from_short_code(cases[i].short_code, strlen(cases[i].short_code),
                                          &product_id) == EZO_OK);
    assert(product_id == cases[i].product_id);

    assert_hint(cases[i].product_id, EZO_COMMAND_GENERIC, 300);
    assert_hint(cases[i].product_id, EZO_COMMAND_READ, cases[i].read_ms);
    assert_hint(cases[i].product_id, EZO_COMMAND_READ_WITH_TEMP_COMP,
                cases[i].read_with_temp_comp_ms);
    assert_hint(cases[i].product_id, EZO_COMMAND_CALIBRATION, cases[i].calibration_ms);
  }
}

static void test_metadata_tier_capabilities(void) {
  enum {
    CONTROL = EZO_PRODUCT_FAMILY_IDENTITY | EZO_PRODUCT_FAMILY_DEVICE_CONTROL |
              EZO_PRODUCT_FAMILY_PROTOCOL_CONTROL,
    NO_CALIBRATION = EZO_PRODUCT_FAMILY_ACQUISITION | CONTROL,
    WITH_CALIBRATION = NO_CALIBRATION | EZO_PRODUCT_FAMILY_CALIBRATION
  };
  static const struct {
    ezo_product_id_t product_id;
    uint32_t capabilities;
    uint32_t families;
  } cases[] = {
      {EZO_PRODUCT_CO2,
       EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION | EZO_PRODUCT_CAP_OUTPUT_SELECTION,
       WITH_CALIBRATION},
      {EZO_PRODUCT_O2,
       EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION |
           EZO_PRODUCT_CAP_TEMPERATURE_COMPENSATION | EZO_PRODUCT_CAP_OUTPUT_SELECTION,
       WITH_CALIBRATION},
      {EZO_PRODUCT_RGB,
       EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION | EZO_PRODUCT_CAP_OUTPUT_SELECTION,
       WITH_CALIBRATION},
      {EZO_PRODUCT_PRS,
       EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_CALIBRATION | EZO_PRODUCT_CAP_SCALE_SELECTION,
       WITH_CALIBRATION},
      {EZO_PRODUCT_FLO,
       EZO_PRODUCT_CAP_MEASUREMENT | EZO_PRODUCT_CAP_OUTPUT_SELECTION |
           EZO_PRODUCT_CAP_PROBE_CONFIGURATION,
       NO_CALIBRATION},
      {EZO_PRODUCT_PMP, EZO_PRODUCT_CAP_CALIBRATION | EZO_PRODUCT_CAP_OUTPUT_SELECTION,
       WITH_CALIBRATION},
      {EZO_PRODUCT_PMP_L, EZO_PRODUCT_CAP_CALIBRATION | EZO_PRODUCT_CAP_OUTPUT_SELECTION,
       WITH_CALIBRATION},
  };
  size_t i = 0;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    const ezo_product_metadata_t *metadata = ezo_product_get_metadata(cases[i].product_id);
    assert(metadata != NULL);
    assert(metadata->capability_flags == cases[i].capabilities);
    assert(metadata->command_family_flags == cases[i].families);
  }
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

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_EC,
                                     EZO_PRODUCT_TRANSPORT_UART,
                                     EZO_COMMAND_CALIBRATION,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 600);

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_DO,
                                     EZO_PRODUCT_TRANSPORT_UART,
                                     EZO_COMMAND_CALIBRATION,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 1300);

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_ORP,
                                     EZO_PRODUCT_TRANSPORT_UART,
                                     EZO_COMMAND_CALIBRATION,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 900);

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_RTD,
                                     EZO_PRODUCT_TRANSPORT_I2C,
                                     EZO_COMMAND_READ,
                                     &hint) == EZO_OK);
  assert(hint.wait_ms == 600);

  assert(ezo_product_get_timing_hint(EZO_PRODUCT_RTD,
                                     EZO_PRODUCT_TRANSPORT_UART,
                                     EZO_COMMAND_CALIBRATION,
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

static ezo_firmware_version_t parsed_version(const char *text) {
  ezo_firmware_version_t version;
  assert(ezo_parse_firmware_version(text, strlen(text), &version) == EZO_OK);
  return version;
}

static void test_parse_firmware_version_reads_tenths_and_hundredths(void) {
  static const struct {
    const char *text;
    uint16_t major;
    uint16_t minor_hundredths;
  } cases[] = {
      {"2.16", 2, 16}, {"1.98", 1, 98}, {"1.1", 1, 10},   {"1.10", 1, 10}, {"1.01", 1, 1},
      {"1.0", 1, 0},   {"1.00", 1, 0},  {"19.7", 19, 70}, {"0.5", 0, 50},  {"65535.99", 65535, 99},
  };
  size_t i = 0;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    ezo_firmware_version_t version = parsed_version(cases[i].text);
    assert(version.major == cases[i].major);
    assert(version.minor_hundredths == cases[i].minor_hundredths);
  }
}

static void test_parse_firmware_version_rejects_other_text(void) {
  static const char *const cases[] = {"",    "2",    "2.",     ".5",       "2.x",   "x.2", "2.123",
                                      "2.1.3", " 2.1", "2.1 ", "-1.0", "1.+", "65536.0"};
  ezo_firmware_version_t version;
  size_t i = 0;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    assert(ezo_parse_firmware_version(cases[i], strlen(cases[i]), &version) == EZO_ERR_PARSE);
  }
  assert(ezo_parse_firmware_version(NULL, 0, &version) == EZO_ERR_INVALID_ARGUMENT);
  assert(ezo_parse_firmware_version("2.16", 4, NULL) == EZO_ERR_INVALID_ARGUMENT);
}

static void test_firmware_version_compare_is_decimal(void) {
  static const struct {
    const char *lower;
    const char *higher;
  } ordered[] = {
      {"1.05", "1.1"}, {"1.01", "1.1"},  {"1.7", "1.95"}, {"2.01", "2.10"},
      {"1.98", "2.10"}, {"2.15", "2.16"}, {"9.99", "10.0"},
  };
  size_t i = 0;

  for (i = 0; i < sizeof(ordered) / sizeof(ordered[0]); ++i) {
    ezo_firmware_version_t lower = parsed_version(ordered[i].lower);
    ezo_firmware_version_t higher = parsed_version(ordered[i].higher);
    assert(ezo_firmware_version_compare(&lower, &higher) < 0);
    assert(ezo_firmware_version_compare(&higher, &lower) > 0);
  }

  {
    ezo_firmware_version_t tenths = parsed_version("1.1");
    ezo_firmware_version_t hundredths = parsed_version("1.10");
    assert(ezo_firmware_version_compare(&tenths, &hundredths) == 0);
  }
}

static void test_firmware_feature_thresholds(void) {
  static const struct {
    ezo_product_id_t product;
    ezo_firmware_feature_t feature;
    const char *below;
    const char *at;
  } cases[] = {
      {EZO_PRODUCT_PH, EZO_FIRMWARE_FEATURE_PROTOCOL_LOCK, "1.94", "1.95"},
      {EZO_PRODUCT_PH, EZO_FIRMWARE_FEATURE_FIND, "2.09", "2.10"},
      {EZO_PRODUCT_PH, EZO_FIRMWARE_FEATURE_CALIBRATION_TRANSFER, "2.09", "2.10"},
      {EZO_PRODUCT_PH, EZO_FIRMWARE_FEATURE_READ_WITH_TEMP_COMP, "2.11", "2.12"},
      {EZO_PRODUCT_PH, EZO_FIRMWARE_FEATURE_EXTENDED_RANGE, "2.13", "2.14"},
      {EZO_PRODUCT_ORP, EZO_FIRMWARE_FEATURE_PROTOCOL_LOCK, "1.94", "1.95"},
      {EZO_PRODUCT_ORP, EZO_FIRMWARE_FEATURE_FIND, "2.09", "2.10"},
      {EZO_PRODUCT_ORP, EZO_FIRMWARE_FEATURE_CALIBRATION_TRANSFER, "2.09", "2.10"},
      {EZO_PRODUCT_ORP, EZO_FIRMWARE_FEATURE_EXTENDED_RANGE, "2.13", "2.14"},
      {EZO_PRODUCT_EC, EZO_FIRMWARE_FEATURE_PROTOCOL_LOCK, "1.94", "1.95"},
      {EZO_PRODUCT_EC, EZO_FIRMWARE_FEATURE_FIND, "2.09", "2.10"},
      {EZO_PRODUCT_EC, EZO_FIRMWARE_FEATURE_CALIBRATION_TRANSFER, "2.09", "2.10"},
      {EZO_PRODUCT_EC, EZO_FIRMWARE_FEATURE_READ_WITH_TEMP_COMP, "2.12", "2.13"},
      {EZO_PRODUCT_DO, EZO_FIRMWARE_FEATURE_PROTOCOL_LOCK, "1.94", "1.95"},
      {EZO_PRODUCT_DO, EZO_FIRMWARE_FEATURE_FIND, "2.09", "2.10"},
      {EZO_PRODUCT_DO, EZO_FIRMWARE_FEATURE_CALIBRATION_TRANSFER, "2.09", "2.10"},
      {EZO_PRODUCT_DO, EZO_FIRMWARE_FEATURE_READ_WITH_TEMP_COMP, "2.12", "2.13"},
      {EZO_PRODUCT_RTD, EZO_FIRMWARE_FEATURE_PROTOCOL_LOCK, "1.01", "1.02"},
      {EZO_PRODUCT_RTD, EZO_FIRMWARE_FEATURE_FIND, "2.09", "2.10"},
      {EZO_PRODUCT_RTD, EZO_FIRMWARE_FEATURE_CALIBRATION_TRANSFER, "2.09", "2.10"},
      {EZO_PRODUCT_HUM, EZO_FIRMWARE_FEATURE_PROTOCOL_LOCK, "0.99", "1.00"},
      {EZO_PRODUCT_HUM, EZO_FIRMWARE_FEATURE_FIND, "0.99", "1.00"},
      {EZO_PRODUCT_HUM, EZO_FIRMWARE_FEATURE_TEMPERATURE_CALIBRATION, "1.00", "1.01"},
  };
  size_t i = 0;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    ezo_firmware_version_t below = parsed_version(cases[i].below);
    ezo_firmware_version_t at = parsed_version(cases[i].at);
    ezo_firmware_version_t newer = parsed_version("9.99");
    assert(!ezo_product_firmware_has_feature(cases[i].product, cases[i].feature, &below));
    assert(ezo_product_firmware_has_feature(cases[i].product, cases[i].feature, &at));
    assert(ezo_product_firmware_has_feature(cases[i].product, cases[i].feature, &newer));
  }
}

static void test_firmware_feature_absent_from_product_is_false(void) {
  ezo_firmware_version_t newest = parsed_version("9.99");
  ezo_firmware_version_t hum_c = parsed_version("1.1");

  assert(!ezo_product_firmware_has_feature(EZO_PRODUCT_ORP,
                                           EZO_FIRMWARE_FEATURE_READ_WITH_TEMP_COMP, &newest));
  assert(!ezo_product_firmware_has_feature(EZO_PRODUCT_EC, EZO_FIRMWARE_FEATURE_EXTENDED_RANGE,
                                           &newest));
  assert(!ezo_product_firmware_has_feature(EZO_PRODUCT_HUM,
                                           EZO_FIRMWARE_FEATURE_CALIBRATION_TRANSFER, &newest));
  assert(!ezo_product_firmware_has_feature(EZO_PRODUCT_PH,
                                           EZO_FIRMWARE_FEATURE_TEMPERATURE_CALIBRATION, &newest));
  assert(!ezo_product_firmware_has_feature(EZO_PRODUCT_UNKNOWN, EZO_FIRMWARE_FEATURE_FIND,
                                           &newest));
  assert(!ezo_product_firmware_has_feature(EZO_PRODUCT_PH, EZO_FIRMWARE_FEATURE_UNKNOWN, &newest));
  assert(!ezo_product_firmware_has_feature(EZO_PRODUCT_PH, EZO_FIRMWARE_FEATURE_FIND, NULL));

  /* The HUM-C changelog spells this release 1.1, the probe changelog 1.01. */
  assert(ezo_product_firmware_has_feature(EZO_PRODUCT_HUM,
                                          EZO_FIRMWARE_FEATURE_TEMPERATURE_CALIBRATION, &hum_c));
}

static void test_firmware_issue_thresholds(void) {
  static const struct {
    ezo_product_id_t product;
    ezo_firmware_issue_t issue;
    const char *affected;
    const char *fixed;
  } cases[] = {
      {EZO_PRODUCT_PH, EZO_FIRMWARE_ISSUE_I2C_SLEEP_WAKE, "2.15", "2.16"},
      {EZO_PRODUCT_ORP, EZO_FIRMWARE_ISSUE_I2C_SLEEP_WAKE, "2.12", "2.13"},
      {EZO_PRODUCT_EC, EZO_FIRMWARE_ISSUE_I2C_SLEEP_WAKE, "2.11", "2.12"},
      {EZO_PRODUCT_RTD, EZO_FIRMWARE_ISSUE_FIRST_READING_INVALID, "2.10", "2.11"},
  };
  size_t i = 0;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    ezo_firmware_version_t affected = parsed_version(cases[i].affected);
    ezo_firmware_version_t fixed = parsed_version(cases[i].fixed);
    ezo_firmware_version_t newer = parsed_version("9.99");
    assert(ezo_product_firmware_has_issue(cases[i].product, cases[i].issue, &affected));
    assert(!ezo_product_firmware_has_issue(cases[i].product, cases[i].issue, &fixed));
    assert(!ezo_product_firmware_has_issue(cases[i].product, cases[i].issue, &newer));
  }
}

static void test_firmware_issue_absent_from_product_is_false(void) {
  ezo_firmware_version_t oldest = parsed_version("0.00");

  assert(!ezo_product_firmware_has_issue(EZO_PRODUCT_DO, EZO_FIRMWARE_ISSUE_I2C_SLEEP_WAKE,
                                         &oldest));
  assert(!ezo_product_firmware_has_issue(EZO_PRODUCT_RTD, EZO_FIRMWARE_ISSUE_I2C_SLEEP_WAKE,
                                         &oldest));
  assert(!ezo_product_firmware_has_issue(EZO_PRODUCT_PH, EZO_FIRMWARE_ISSUE_FIRST_READING_INVALID,
                                         &oldest));
  assert(!ezo_product_firmware_has_issue(EZO_PRODUCT_UNKNOWN, EZO_FIRMWARE_ISSUE_I2C_SLEEP_WAKE,
                                         &oldest));
  assert(!ezo_product_firmware_has_issue(EZO_PRODUCT_PH, EZO_FIRMWARE_ISSUE_UNKNOWN, &oldest));
  assert(!ezo_product_firmware_has_issue(EZO_PRODUCT_PH, EZO_FIRMWARE_ISSUE_I2C_SLEEP_WAKE, NULL));
}

int main(void) {
  test_parse_device_info_for_initial_products();
  test_parse_device_info_accepts_unrecognized_product_code();
  test_parse_device_info_accepts_observed_uppercase_identity_variant();
  test_parse_device_info_rejects_non_identity_payloads();
  test_product_lookup_accepts_normalized_short_codes();
  test_product_metadata_registry_exposes_defaults_and_support_tiers();
  test_product_lookup_by_short_code_returns_metadata();
  test_product_capability_and_family_lookups();
  test_product_timing_lookup_uses_transport_specific_profiles();
  test_parse_firmware_version_reads_tenths_and_hundredths();
  test_parse_firmware_version_rejects_other_text();
  test_firmware_version_compare_is_decimal();
  test_firmware_feature_thresholds();
  test_firmware_feature_absent_from_product_is_false();
  test_firmware_issue_thresholds();
  test_firmware_issue_absent_from_product_is_false();
  test_parse_device_info_identifies_metadata_tier_products();
  test_metadata_tier_products_record_their_facts();
  test_metadata_tier_capabilities();
  return 0;
}
