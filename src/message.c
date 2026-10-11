#include "../include/message.h"

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define MEASUREMENT_SIZE 21

#define TEMP_VALID     0x01u
#define PRESSURE_VALID 0x02u
#define HUMIDITY_VALID 0x04u
#define VALID_MASK     0x07u

_Static_assert(sizeof(float) == 4, "float must be 32 bits");
_Static_assert(FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128,
               "Protocol requires IEEE-754 binary32");

static uint32_t read_u32_le(const uint8_t *src) {
  return ((uint32_t)src[0]) |
         ((uint32_t)src[1] << 8) |
         ((uint32_t)src[2] << 16) |
         ((uint32_t)src[3] << 24);
}

static void write_u32_le(uint8_t *dst, uint32_t value) {
  dst[0] = (uint8_t)value;
  dst[1] = (uint8_t)(value >> 8);
  dst[2] = (uint8_t)(value >> 16);
  dst[3] = (uint8_t)(value >> 24);
}

static uint32_t float_to_bits(float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  return bits;
}

static float bits_to_float(uint32_t bits) {
  float value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

int is_positive_zero(float value) {
  return float_to_bits(value) == 0u;
}

/*
 * Encoding allows finite values in disabled channels because
 * they will be normalized to +0.0f on the wire.
 *
 * Decoding requires disabled channels to already contain +0.0f.
 */
static enum message_error validate_measurement(const struct measurement *m, int require_disabled_zero) {
  if(m == NULL) {
    return MSG_BAD_VALUE;
  }

  uint8_t mask = m->valid_mask;

  if(mask == 0 || (mask & ~VALID_MASK) != 0) {
    return MSG_BAD_VALUE;
  }

  if(!isfinite(m->temperature_c) ||
     !isfinite(m->pressure_pa) ||
     !isfinite(m->humidity_rh)) {
    return MSG_BAD_VALUE;
  }

  if(mask & TEMP_VALID) {
    if(m->temperature_c < -40.0f ||
       m->temperature_c > 85.0f) {
      return MSG_BAD_VALUE;
    }
  } else if(require_disabled_zero &&
            !is_positive_zero(m->temperature_c)) {
    return MSG_BAD_VALUE;
  }

  if(mask & PRESSURE_VALID) {
    if(m->pressure_pa < 30000.0f ||
       m->pressure_pa > 110000.0f) {
      return MSG_BAD_VALUE;
    }
  } else if(require_disabled_zero &&
            !is_positive_zero(m->pressure_pa)) {
    return MSG_BAD_VALUE;
  }

  if(mask & HUMIDITY_VALID) {
    if(m->humidity_rh < 0.0f ||
       m->humidity_rh > 100.0f) {
      return MSG_BAD_VALUE;
    }
  } else if(require_disabled_zero &&
            !is_positive_zero(m->humidity_rh)) {
    return MSG_BAD_VALUE;
  }

  return MSG_OK;
}

enum message_error is_invalid_measurement(struct measurement *measurement) {
  return validate_measurement(measurement, 1);
}

enum message_error measurement_encode(struct measurement *measurement, uint8_t out[MEASUREMENT_SIZE]) {
  if(out == NULL) {
    return MSG_BAD_VALUE;
  }

  enum message_error error = validate_measurement(measurement, 0);

  if(error != MSG_OK) {
    return error;
  }

  uint8_t mask = measurement->valid_mask;

  write_u32_le(&out[0], measurement->uptime_ms);
  write_u32_le(&out[4], measurement->sample_id);

  out[8] = mask;

  write_u32_le(&out[9],
      (mask & TEMP_VALID)
          ? float_to_bits(measurement->temperature_c)
          : 0u);

  write_u32_le(&out[13],
      (mask & PRESSURE_VALID)
          ? float_to_bits(measurement->pressure_pa)
          : 0u);

  write_u32_le(&out[17],
      (mask & HUMIDITY_VALID)
          ? float_to_bits(measurement->humidity_rh)
          : 0u);

  return MSG_OK;
}

enum message_error measurement_decode(
    const uint8_t *payload,
    size_t len,
    struct measurement *out) {

  if(len != MEASUREMENT_SIZE) {
    return MSG_BAD_LENGTH;
  }

  if(payload == NULL || out == NULL) {
    return MSG_BAD_VALUE;
  }

  struct measurement decoded = {0};

  decoded.uptime_ms = read_u32_le(&payload[0]);
  decoded.sample_id = read_u32_le(&payload[4]);
  decoded.valid_mask = payload[8];

  decoded.temperature_c =
      bits_to_float(read_u32_le(&payload[9]));

  decoded.pressure_pa =
      bits_to_float(read_u32_le(&payload[13]));

  decoded.humidity_rh =
      bits_to_float(read_u32_le(&payload[17]));

  enum message_error error = is_invalid_measurement(&decoded);

  if(error != MSG_OK) {
    return error;
  }

  /*
   * Only modify the caller's output once every field
   * has passed validation.
   */
  *out = decoded;

  return MSG_OK;
}
