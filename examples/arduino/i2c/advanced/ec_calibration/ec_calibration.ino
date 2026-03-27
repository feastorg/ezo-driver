/*
Purpose: stage EC dry/single/low/high calibration with explicit probe-K and temperature-state inspection.
Defaults: Wire on the board default pins, address 100, dry step, probe K 1.0, and reference 84.0 uS.
Assumptions: dry calibration happens first and temperature compensation remains at the vendor default during calibration.
Next: read ../ec_workflow/ec_workflow.ino for output selection, probe-K, and TDS-factor state.
*/

#include <ezo_arduino_i2c_example.hpp>

#include <ezo_ec.h>

enum CalibrationStep {
  STEP_STATUS = 0,
  STEP_DRY,
  STEP_SINGLE,
  STEP_LOW,
  STEP_HIGH,
  STEP_CLEAR
};

static const unsigned long STARTUP_SETTLE_MS = 3000UL;
static const uint8_t DEVICE_I2C_ADDRESS = 100U;
static const uint8_t APPLY_CHANGES = 0U;
static const CalibrationStep CALIBRATION_STEP = STEP_STATUS;
static const float PROBE_K = 1.0f;
static const float REFERENCE_US = 84.0f;

static ezo_arduino_wire_context_t wire_context;
static ezo_i2c_device_t device;
static unsigned long startup_started_at_ms = 0;
static uint8_t calibration_done = 0U;

static ezo_ec_calibration_point_t calibration_point() {
  switch (CALIBRATION_STEP) {
    case STEP_SINGLE:
      return EZO_EC_CALIBRATION_SINGLE_POINT;
    case STEP_LOW:
      return EZO_EC_CALIBRATION_LOW_POINT;
    case STEP_HIGH:
      return EZO_EC_CALIBRATION_HIGH_POINT;
    case STEP_DRY:
    case STEP_CLEAR:
    case STEP_STATUS:
    default:
      return EZO_EC_CALIBRATION_DRY;
  }
}

static void run_workflow() {
  ezo_timing_hint_t hint;
  ezo_ec_temperature_compensation_t temperature;
  ezo_ec_probe_k_t probe_k;
  ezo_ec_calibration_status_t calibration;

  EZO_ARDUINO_CHECK_OK("send_temperature_query",
                       ezo_ec_send_temperature_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_temperature_query",
                       ezo_ec_read_temperature_i2c(&device, &temperature));

  EZO_ARDUINO_CHECK_OK("send_probe_k_query", ezo_ec_send_probe_k_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_probe_k_query", ezo_ec_read_probe_k_i2c(&device, &probe_k));

  EZO_ARDUINO_CHECK_OK("send_calibration_query",
                       ezo_ec_send_calibration_query_i2c(&device, &hint));
  ezo_arduino_wait_hint(&hint);
  EZO_ARDUINO_CHECK_OK("read_calibration_query",
                       ezo_ec_read_calibration_status_i2c(&device, &calibration));

  Serial.println(F("transport=i2c"));
  Serial.print(F("address="));
  Serial.println(DEVICE_I2C_ADDRESS);
  Serial.print(F("current_temperature_compensation_c="));
  Serial.println(temperature.temperature_c, 3);
  Serial.print(F("current_probe_k="));
  Serial.println(probe_k.k_value, 3);
  Serial.print(F("current_calibration_level="));
  Serial.println((unsigned long)calibration.level);
  Serial.println(F("vendor_guidance_dry_first=1"));
  Serial.println(F("vendor_guidance_keep_temp_comp_default_during_calibration=1"));
  Serial.print(F("apply_changes="));
  Serial.println((unsigned)APPLY_CHANGES);
  Serial.print(F("planned_probe_k="));
  Serial.println(PROBE_K, 3);
  Serial.print(F("planned_reference_us="));
  Serial.println(REFERENCE_US, 3);

  if (APPLY_CHANGES == 0U) {
    return;
  }

  EZO_ARDUINO_CHECK_OK("send_probe_k_set",
                       ezo_ec_send_probe_k_set_i2c(&device, PROBE_K, 2U, &hint));
  ezo_arduino_wait_hint(&hint);

  if (CALIBRATION_STEP == STEP_CLEAR) {
    EZO_ARDUINO_CHECK_OK("send_clear_calibration",
                         ezo_ec_send_clear_calibration_i2c(&device, &hint));
  } else if (CALIBRATION_STEP != STEP_STATUS) {
    EZO_ARDUINO_CHECK_OK("send_calibration",
                         ezo_ec_send_calibration_i2c(
                             &device, calibration_point(), REFERENCE_US, 2U, &hint));
  }
  if (CALIBRATION_STEP != STEP_STATUS) {
    ezo_arduino_wait_hint(&hint);
  }
}

void setup() {
  ezo_arduino_i2c_begin();
  EZO_ARDUINO_CHECK_OK("init_wire_context", ezo_arduino_i2c_init_context(&wire_context));
  EZO_ARDUINO_CHECK_OK("init_device",
                       ezo_arduino_i2c_init_device(&device, DEVICE_I2C_ADDRESS, &wire_context));
  startup_started_at_ms = millis();
}

void loop() {
  if (calibration_done != 0U) {
    return;
  }

  if (!ezo_arduino_startup_elapsed(startup_started_at_ms, STARTUP_SETTLE_MS)) {
    return;
  }

  run_workflow();
  calibration_done = 1U;
}
