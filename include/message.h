#include <stddef.h>
#include <stdint.h>

struct measurement {
  uint32_t uptime_ms;
  uint32_t sample_id;
  uint8_t valid_mask;
  float temperature_c;
  float pressure_pa;
  float humidity_rh;
};

enum message_error {
  MSG_OK, 
  MSG_BAD_TYPE, 
  MSG_BAD_LENGTH, 
  MSG_BAD_VALUE, 
  MSG_UNSUPPORTED,
};

enum message_error measurement_encode(struct measurement *measurement, uint8_t out[21]);
enum message_error measurement_decode(const uint8_t *payload, size_t len, struct measurement *out);
