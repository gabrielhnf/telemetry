#include "unity/unity.h"
#include "../include/message.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MEASUREMENT_PAYLOAD_SIZE 21

/* Independent, byte-exact vector from spec/test_vectors.json (measurement_all). */
static const uint8_t GOLDEN_ALL[MEASUREMENT_PAYLOAD_SIZE] = {
  0xB0, 0x04, 0x00, 0x00,       /* uptime_ms = 1200 */
  0x05, 0x00, 0x00, 0x00,       /* sample_id = 5 */
  0x07,                         /* all three channels valid */
  0x00, 0x00, 0xBE, 0x41,       /* temperature = 23.75 C */
  0x80, 0xE6, 0xC5, 0x47,       /* pressure = 101325 Pa */
  0x00, 0x00, 0x42, 0x42        /* humidity = 48.5 %RH */
};

static const uint8_t GOLDEN_HUMIDITY_ONLY[MEASUREMENT_PAYLOAD_SIZE] = {
  0x01, 0x00, 0x00, 0x00,       /* uptime_ms = 1 */
  0x02, 0x00, 0x00, 0x00,       /* sample_id = 2 */
  0x04,                         /* only humidity valid */
  0x00, 0x00, 0x00, 0x00,       /* temperature unavailable => +0 */
  0x00, 0x00, 0x00, 0x00,       /* pressure unavailable => +0 */
  0x00, 0x00, 0x80, 0x3F        /* humidity = 1.0 %RH */
};

static struct measurement make_measurement(uint8_t mask) {
  struct measurement m = {
    .uptime_ms = 1200,
    .sample_id = 5,
    .valid_mask = mask,
    .temperature_c = 23.75f,
    .pressure_pa = 101325.0f,
    .humidity_rh = 48.5f
  };
  return m;
}

static void assert_measurement(const struct measurement *m,
                               uint32_t uptime, uint32_t sample,
                               uint8_t mask, float temp,
                               float pressure, float humidity) {
  TEST_ASSERT_EQUAL_UINT32(uptime, m->uptime_ms);
  TEST_ASSERT_EQUAL_UINT32(sample, m->sample_id);
  TEST_ASSERT_EQUAL_HEX8(mask, m->valid_mask);
  TEST_ASSERT_EQUAL_FLOAT(temp, m->temperature_c);
  TEST_ASSERT_EQUAL_FLOAT(pressure, m->pressure_pa);
  TEST_ASSERT_EQUAL_FLOAT(humidity, m->humidity_rh);
}

static struct measurement sentinel(void) {
  struct measurement m = {
    .uptime_ms = 0x11223344u,
    .sample_id = 0x55667788u,
    .valid_mask = 0x5A,
    .temperature_c = 12.5f,
    .pressure_pa = 45678.0f,
    .humidity_rh = 61.25f
  };
  return m;
}

static void assert_decode_error_unchanged(const uint8_t *bytes, size_t len,
                                          enum message_error expected_error) {
  struct measurement out = sentinel();

  TEST_ASSERT_EQUAL_INT(expected_error,
                        measurement_decode(bytes, len, &out));
  assert_measurement(&out, 0x11223344u, 0x55667788u, 0x5A,
                     12.5f, 45678.0f, 61.25f);
}

void setUp(void) {
}

void tearDown(void) {
}

void test_measurement_encode_matches_golden_bytes(void) {
  struct measurement m = make_measurement(0x07);
  uint8_t out[MEASUREMENT_PAYLOAD_SIZE] = {0};

  TEST_ASSERT_EQUAL_INT(MSG_OK, measurement_encode(&m, out));
  TEST_ASSERT_EQUAL_HEX8_ARRAY(GOLDEN_ALL, out, sizeof(out));
}

void test_measurement_decode_matches_golden_values(void) {
  struct measurement out = {0};

  TEST_ASSERT_EQUAL_INT(MSG_OK,
                        measurement_decode(GOLDEN_ALL, sizeof(GOLDEN_ALL), &out));
  assert_measurement(&out, 1200, 5, 0x07, 23.75f, 101325.0f, 48.5f);
}

void test_measurement_encode_normalizes_disabled_channels(void) {
  /* Deliberately identical to your original {1, 2, 4, 1, 1, 1} example. */
  struct measurement m = {1, 2, 4, 1.0f, 1.0f, 1.0f};
  uint8_t out[MEASUREMENT_PAYLOAD_SIZE] = {0};

  TEST_ASSERT_EQUAL_INT(MSG_OK, measurement_encode(&m, out));
  TEST_ASSERT_EQUAL_HEX8_ARRAY(GOLDEN_HUMIDITY_ONLY, out, sizeof(out));

  /* Encoding must not modify the caller's input structure. */
  assert_measurement(&m, 1, 2, 0x04, 1.0f, 1.0f, 1.0f);
}

void test_measurement_decode_humidity_only(void) {
  struct measurement out = {0};

  TEST_ASSERT_EQUAL_INT(MSG_OK,
                        measurement_decode(GOLDEN_HUMIDITY_ONLY,
                                           sizeof(GOLDEN_HUMIDITY_ONLY), &out));
  assert_measurement(&out, 1, 2, 0x04, 0.0f, 0.0f, 1.0f);
}

void test_measurement_encode_writes_exactly_21_bytes(void) {
  struct measurement m = make_measurement(0x07);
  uint8_t guarded[MEASUREMENT_PAYLOAD_SIZE + 2];
  memset(guarded, 0xA5, sizeof(guarded));

  TEST_ASSERT_EQUAL_INT(MSG_OK, measurement_encode(&m, &guarded[1]));
  TEST_ASSERT_EQUAL_HEX8(0xA5, guarded[0]);
  TEST_ASSERT_EQUAL_HEX8(0xA5, guarded[MEASUREMENT_PAYLOAD_SIZE + 1]);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(GOLDEN_ALL, &guarded[1],
                               MEASUREMENT_PAYLOAD_SIZE);
}

void test_measurement_roundtrip_with_only_temperature_and_pressure(void) {
  struct measurement m = make_measurement(0x03);
  struct measurement out = {0};
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE] = {0};

  TEST_ASSERT_EQUAL_INT(MSG_OK, measurement_encode(&m, bytes));
  TEST_ASSERT_EQUAL_HEX8(0x03, bytes[8]);
  TEST_ASSERT_EQUAL_HEX8(0x00, bytes[17]);
  TEST_ASSERT_EQUAL_HEX8(0x00, bytes[18]);
  TEST_ASSERT_EQUAL_HEX8(0x00, bytes[19]);
  TEST_ASSERT_EQUAL_HEX8(0x00, bytes[20]);
  TEST_ASSERT_EQUAL_INT(MSG_OK,
                        measurement_decode(bytes, sizeof(bytes), &out));
  assert_measurement(&out, 1200, 5, 0x03, 23.75f, 101325.0f, 0.0f);
}

void test_measurement_roundtrip_with_large_counters(void) {
  struct measurement m = make_measurement(0x07);
  struct measurement out = {0};
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE] = {0};

  m.uptime_ms = 0x12345678u;
  m.sample_id = 0xABCDEF01u;

  TEST_ASSERT_EQUAL_INT(MSG_OK, measurement_encode(&m, bytes));
  TEST_ASSERT_EQUAL_HEX8(0x78, bytes[0]);
  TEST_ASSERT_EQUAL_HEX8(0x56, bytes[1]);
  TEST_ASSERT_EQUAL_HEX8(0x34, bytes[2]);
  TEST_ASSERT_EQUAL_HEX8(0x12, bytes[3]);
  TEST_ASSERT_EQUAL_HEX8(0x01, bytes[4]);
  TEST_ASSERT_EQUAL_HEX8(0xEF, bytes[5]);
  TEST_ASSERT_EQUAL_HEX8(0xCD, bytes[6]);
  TEST_ASSERT_EQUAL_HEX8(0xAB, bytes[7]);

  TEST_ASSERT_EQUAL_INT(MSG_OK,
                        measurement_decode(bytes, sizeof(bytes), &out));
  assert_measurement(&out, 0x12345678u, 0xABCDEF01u, 0x07,
                     23.75f, 101325.0f, 48.5f);
}

void test_measurement_encode_rejects_zero_and_reserved_masks(void) {
  struct measurement m = make_measurement(0x00);
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE] = {0};

  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
  m.valid_mask = 0x08;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
  m.valid_mask = 0xFF;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
}

void test_measurement_decode_rejects_zero_and_reserved_masks(void) {
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE];
  memcpy(bytes, GOLDEN_ALL, sizeof(bytes));

  bytes[8] = 0x00;
  assert_decode_error_unchanged(bytes, sizeof(bytes), MSG_BAD_VALUE);
  bytes[8] = 0x08;
  assert_decode_error_unchanged(bytes, sizeof(bytes), MSG_BAD_VALUE);
  bytes[8] = 0xFF;
  assert_decode_error_unchanged(bytes, sizeof(bytes), MSG_BAD_VALUE);
}

void test_measurement_encode_rejects_nonfinite_values(void) {
  struct measurement m = make_measurement(0x07);
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE] = {0};

  m.temperature_c = NAN;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
  m = make_measurement(0x07);
  m.pressure_pa = INFINITY;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
  m = make_measurement(0x07);
  m.humidity_rh = -INFINITY;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));

  /* Even a masked-off channel MUST have a finite input to the encoder. */
  m = make_measurement(0x04);
  m.temperature_c = NAN;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
}

void test_measurement_encode_rejects_out_of_range_enabled_values(void) {
  struct measurement m = make_measurement(0x07);
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE] = {0};

  m.temperature_c = -40.01f;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
  m = make_measurement(0x07);
  m.temperature_c = 85.01f;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
  m = make_measurement(0x07);
  m.pressure_pa = 29999.0f;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
  m = make_measurement(0x07);
  m.pressure_pa = 110001.0f;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
  m = make_measurement(0x07);
  m.humidity_rh = -0.01f;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
  m = make_measurement(0x07);
  m.humidity_rh = 100.01f;
  TEST_ASSERT_EQUAL_INT(MSG_BAD_VALUE, measurement_encode(&m, bytes));
}

void test_measurement_encode_accepts_inclusive_range_boundaries(void) {
  struct measurement m = make_measurement(0x07);
  struct measurement out = {0};
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE] = {0};

  m.temperature_c = -40.0f;
  m.pressure_pa = 30000.0f;
  m.humidity_rh = 0.0f;
  TEST_ASSERT_EQUAL_INT(MSG_OK, measurement_encode(&m, bytes));
  TEST_ASSERT_EQUAL_INT(MSG_OK,
                        measurement_decode(bytes, sizeof(bytes), &out));
  assert_measurement(&out, 1200, 5, 0x07, -40.0f, 30000.0f, 0.0f);

  m.temperature_c = 85.0f;
  m.pressure_pa = 110000.0f;
  m.humidity_rh = 100.0f;
  TEST_ASSERT_EQUAL_INT(MSG_OK, measurement_encode(&m, bytes));
  TEST_ASSERT_EQUAL_INT(MSG_OK,
                        measurement_decode(bytes, sizeof(bytes), &out));
  assert_measurement(&out, 1200, 5, 0x07, 85.0f, 110000.0f, 100.0f);
}

void test_measurement_decode_rejects_short_and_long_payloads(void) {
  uint8_t longer[MEASUREMENT_PAYLOAD_SIZE + 1];
  memcpy(longer, GOLDEN_ALL, MEASUREMENT_PAYLOAD_SIZE);
  longer[MEASUREMENT_PAYLOAD_SIZE] = 0x00;

  assert_decode_error_unchanged(GOLDEN_ALL, 0, MSG_BAD_LENGTH);
  assert_decode_error_unchanged(GOLDEN_ALL, 20, MSG_BAD_LENGTH);
  assert_decode_error_unchanged(longer, sizeof(longer), MSG_BAD_LENGTH);
}

void test_measurement_decode_rejects_nan_in_enabled_channel(void) {
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE];
  memcpy(bytes, GOLDEN_ALL, sizeof(bytes));

  /* Override temperature with 0x7FC00000 (quiet NaN), little-endian. */
  bytes[9]  = 0x00;
  bytes[10] = 0x00;
  bytes[11] = 0xC0;
  bytes[12] = 0x7F;

  assert_decode_error_unchanged(bytes, sizeof(bytes), MSG_BAD_VALUE);
}

void test_measurement_decode_rejects_out_of_range_channel(void) {
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE];
  memcpy(bytes, GOLDEN_ALL, sizeof(bytes));

  /* Override temperature with 100.0f (0x42C80000), outside -40..85. */
  bytes[9]  = 0x00;
  bytes[10] = 0x00;
  bytes[11] = 0xC8;
  bytes[12] = 0x42;

  assert_decode_error_unchanged(bytes, sizeof(bytes), MSG_BAD_VALUE);
}

void test_measurement_decode_rejects_nonzero_disabled_field(void) {
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE];
  memcpy(bytes, GOLDEN_HUMIDITY_ONLY, sizeof(bytes));

  /* Temperature is disabled, but contains 1.0f instead of +0.0f. */
  bytes[11] = 0x80;
  bytes[12] = 0x3F;

  assert_decode_error_unchanged(bytes, sizeof(bytes), MSG_BAD_VALUE);
}

void test_measurement_decode_rejects_negative_zero_disabled_field(void) {
  uint8_t bytes[MEASUREMENT_PAYLOAD_SIZE];
  memcpy(bytes, GOLDEN_HUMIDITY_ONLY, sizeof(bytes));

  /* Temperature is disabled; -0.0f has sign bit set and is forbidden. */
  bytes[12] = 0x80;

  assert_decode_error_unchanged(bytes, sizeof(bytes), MSG_BAD_VALUE);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_measurement_encode_matches_golden_bytes);
  RUN_TEST(test_measurement_decode_matches_golden_values);
  RUN_TEST(test_measurement_encode_normalizes_disabled_channels);
  RUN_TEST(test_measurement_decode_humidity_only);
  RUN_TEST(test_measurement_encode_writes_exactly_21_bytes);
  RUN_TEST(test_measurement_roundtrip_with_only_temperature_and_pressure);
  RUN_TEST(test_measurement_roundtrip_with_large_counters);
  RUN_TEST(test_measurement_encode_rejects_zero_and_reserved_masks);
  RUN_TEST(test_measurement_decode_rejects_zero_and_reserved_masks);
  RUN_TEST(test_measurement_encode_rejects_nonfinite_values);
  RUN_TEST(test_measurement_encode_rejects_out_of_range_enabled_values);
  RUN_TEST(test_measurement_encode_accepts_inclusive_range_boundaries);
  RUN_TEST(test_measurement_decode_rejects_short_and_long_payloads);
  RUN_TEST(test_measurement_decode_rejects_nan_in_enabled_channel);
  RUN_TEST(test_measurement_decode_rejects_out_of_range_channel);
  RUN_TEST(test_measurement_decode_rejects_nonzero_disabled_field);
  RUN_TEST(test_measurement_decode_rejects_negative_zero_disabled_field);

  return UNITY_END();
}
