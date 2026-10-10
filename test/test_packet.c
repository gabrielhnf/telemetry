#include "unity/unity.h"
#include "../include/packet.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

static void free_decoded_packet(struct packet *packet) {
  if(packet == NULL) {
    return;
  }

  free(packet->payload);
  free(packet);
}

void setUp(void) {
}

void tearDown(void) {
}

void test_packet_encode_empty_payload(void) {
  uint8_t expected[] = {
    0x2B,
    0x00,
    0xFF,
    0xFF,
    0xB0,
    0x01
  };

  uint8_t actual[MAX_PACKET_SIZE];

  struct packet packet = {
    .len = 0,
    .seq = 0xFF,
    .type = 0xFF,
    .payload = NULL,
    .crc = 0
  };

  packet_encode_crc(&packet);

  size_t bytes = packet_encode(
      &packet,
      actual,
      sizeof(actual));

  TEST_ASSERT_EQUAL_size_t(MIN_PACKET_SIZE, bytes);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(
      expected,
      actual,
      sizeof(expected));
}

void test_packet_encode_one_byte_payload(void) {
  uint8_t expected[] = {
    0x2B,
    0x01,
    0xFF,
    0xFF,
    0x01,
    0xFC,
    0xB1
  };

  uint8_t payload[] = {
    0x01
  };

  uint8_t actual[MAX_PACKET_SIZE];

  struct packet packet = {
    .len = sizeof(payload),
    .seq = 0xFF,
    .type = 0xFF,
    .payload = payload,
    .crc = 0
  };

  packet_encode_crc(&packet);

  size_t bytes = packet_encode(
      &packet,
      actual,
      sizeof(actual));

  TEST_ASSERT_EQUAL_size_t(sizeof(expected), bytes);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(
      expected,
      actual,
      sizeof(expected));
}

void test_packet_encode_payload_containing_sof(void) {
  uint8_t expected[] = {
    0x2B,
    0x0B,
    0xFF,
    0xFF,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x2B,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0A,
    0xEF,
    0x64
  };

  uint8_t payload[] = {
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x2B,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0A
  };

  uint8_t actual[MAX_PACKET_SIZE];

  struct packet packet = {
    .len = sizeof(payload),
    .seq = 0xFF,
    .type = 0xFF,
    .payload = payload,
    .crc = 0
  };

  packet_encode_crc(&packet);

  size_t bytes = packet_encode(
      &packet,
      actual,
      sizeof(actual));

  TEST_ASSERT_EQUAL_size_t(sizeof(expected), bytes);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(
      expected,
      actual,
      sizeof(expected));
}

void test_packet_encode_sequence_zero(void) {
  uint8_t expected[] = {
    0x2B,
    0x01,
    0x00,
    0x7E,
    0x01,
    0x9C,
    0xE1
  };

  uint8_t payload[] = {
    0x01
  };

  uint8_t actual[MAX_PACKET_SIZE];

  struct packet packet = {
    .len = sizeof(payload),
    .seq = 0x00,
    .type = 0x7E,
    .payload = payload,
    .crc = 0
  };

  packet_encode_crc(&packet);

  size_t bytes = packet_encode(
      &packet,
      actual,
      sizeof(actual));

  TEST_ASSERT_EQUAL_size_t(sizeof(expected), bytes);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(
      expected,
      actual,
      sizeof(expected));
}

void test_packet_encode_sequence_255(void) {
  uint8_t expected[] = {
    0x2B,
    0x01,
    0xFF,
    0x7E,
    0x01,
    0xAC,
    0xD1
  };

  uint8_t payload[] = {
    0x01
  };

  uint8_t actual[MAX_PACKET_SIZE];

  struct packet packet = {
    .len = sizeof(payload),
    .seq = 0xFF,
    .type = 0x7E,
    .payload = payload,
    .crc = 0
  };

  packet_encode_crc(&packet);

  size_t bytes = packet_encode(
      &packet,
      actual,
      sizeof(actual));

  TEST_ASSERT_EQUAL_size_t(sizeof(expected), bytes);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(
      expected,
      actual,
      sizeof(expected));
}

void test_packet_encode_type_zero(void) {
  uint8_t expected[] = {
    0x2B,
    0x01,
    0x42,
    0x00,
    0x01,
    0x28,
    0x60
  };

  uint8_t payload[] = {
    0x01
  };

  uint8_t actual[MAX_PACKET_SIZE];

  struct packet packet = {
    .len = sizeof(payload),
    .seq = 0x42,
    .type = 0x00,
    .payload = payload,
    .crc = 0
  };

  packet_encode_crc(&packet);

  size_t bytes = packet_encode(
      &packet,
      actual,
      sizeof(actual));

  TEST_ASSERT_EQUAL_size_t(sizeof(expected), bytes);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(
      expected,
      actual,
      sizeof(expected));
}

void test_packet_encode_type_255(void) {
  uint8_t expected[] = {
    0x2B,
    0x01,
    0x42,
    0xFF,
    0x01,
    0xD8,
    0x21
  };

  uint8_t payload[] = {
    0x01
  };

  uint8_t actual[MAX_PACKET_SIZE];

  struct packet packet = {
    .len = sizeof(payload),
    .seq = 0x42,
    .type = 0xFF,
    .payload = payload,
    .crc = 0
  };

  packet_encode_crc(&packet);

  size_t bytes = packet_encode(
      &packet,
      actual,
      sizeof(actual));

  TEST_ASSERT_EQUAL_size_t(sizeof(expected), bytes);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(
      expected,
      actual,
      sizeof(expected));
}

void test_packet_encode_maximum_payload(void) {
  uint8_t payload[255];
  uint8_t actual[MAX_PACKET_SIZE];

  for(size_t i = 0; i < sizeof(payload); i++) {
    payload[i] = (uint8_t)i;
  }

  struct packet packet = {
    .len = 255,
    .seq = 0x12,
    .type = 0x34,
    .payload = payload,
    .crc = 0
  };

  packet_encode_crc(&packet);

  size_t bytes = packet_encode(
      &packet,
      actual,
      sizeof(actual));

  TEST_ASSERT_EQUAL_size_t(MAX_PACKET_SIZE, bytes);

  TEST_ASSERT_EQUAL_HEX8(0x2B, actual[0]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, actual[1]);
  TEST_ASSERT_EQUAL_HEX8(0x12, actual[2]);
  TEST_ASSERT_EQUAL_HEX8(0x34, actual[3]);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(
      payload,
      &actual[4],
      sizeof(payload));

  /*
   * Your protocol currently writes the high CRC byte first.
   */
  TEST_ASSERT_EQUAL_HEX8(
      (uint8_t)((packet.crc >> 8) & 0xFF),
      actual[MAX_PACKET_SIZE - 2]);

  TEST_ASSERT_EQUAL_HEX8(
      (uint8_t)(packet.crc & 0xFF),
      actual[MAX_PACKET_SIZE - 1]);
}

void test_packet_decode_known_packet(void) {
  uint8_t frame[] = {
    0x2B,
    0x02,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0x54,
    0x78
  };

  struct packet *packet = packet_decode(
      frame,
      sizeof(frame));

  TEST_ASSERT_NOT_NULL(packet);

  TEST_ASSERT_EQUAL_UINT8(2, packet->len);
  TEST_ASSERT_EQUAL_HEX8(0xFF, packet->seq);
  TEST_ASSERT_EQUAL_HEX8(0xFF, packet->type);

  TEST_ASSERT_NOT_NULL(packet->payload);
  TEST_ASSERT_EQUAL_HEX8(0xFF, packet->payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, packet->payload[1]);

  TEST_ASSERT_EQUAL_HEX16(0x5478, packet->crc);

  free_decoded_packet(packet);
}

void test_packet_decode_zero_length_payload(void) {
  uint8_t frame[] = {
    0x2B,
    0x00,
    0xFF,
    0xFF,
    0xB0,
    0x01
  };

  struct packet *packet = packet_decode(
      frame,
      sizeof(frame));

  TEST_ASSERT_NOT_NULL(packet);

  TEST_ASSERT_EQUAL_UINT8(0, packet->len);
  TEST_ASSERT_EQUAL_HEX8(0xFF, packet->seq);
  TEST_ASSERT_EQUAL_HEX8(0xFF, packet->type);
  TEST_ASSERT_EQUAL_HEX16(0xB001, packet->crc);

  free_decoded_packet(packet);
}

void test_packet_validate_accepts_valid_packet(void) {
  uint8_t payload[] = {
    0xFF,
    0xFF
  };

  struct packet packet = {
    .len = 2,
    .seq = 0xFF,
    .type = 0xFF,
    .payload = payload,
    .crc = 0x5478
  };

  TEST_ASSERT_NOT_EQUAL(
      -1,
      packet_validate(&packet));
}

void test_packet_validate_rejects_corrupted_payload(void) {
  uint8_t payload[] = {
    0xFF,
    0xFF
  };

  struct packet packet = {
    .len = 2,
    .seq = 0xFF,
    .type = 0xFF,
    .payload = payload,
    .crc = 0x5478
  };

  payload[0] ^= 0x01;

  TEST_ASSERT_EQUAL_INT(
      -1,
      packet_validate(&packet));
}

void test_packet_validate_rejects_corrupted_sequence(void) {
  uint8_t payload[] = {
    0xFF,
    0xFF
  };

  struct packet packet = {
    .len = 2,
    .seq = 0xFF,
    .type = 0xFF,
    .payload = payload,
    .crc = 0x5478
  };

  packet.seq ^= 0x01;

  TEST_ASSERT_EQUAL_INT(
      -1,
      packet_validate(&packet));
}

void test_packet_validate_rejects_corrupted_type(void) {
  uint8_t payload[] = {
    0xFF,
    0xFF
  };

  struct packet packet = {
    .len = 2,
    .seq = 0xFF,
    .type = 0xFF,
    .payload = payload,
    .crc = 0x5478
  };

  packet.type ^= 0x01;

  TEST_ASSERT_EQUAL_INT(
      -1,
      packet_validate(&packet));
}

void test_packet_validate_rejects_corrupted_crc(void) {
  uint8_t payload[] = {
    0xFF,
    0xFF
  };

  struct packet packet = {
    .len = 2,
    .seq = 0xFF,
    .type = 0xFF,
    .payload = payload,
    .crc = 0x5478
  };

  packet.crc ^= 0x0001;

  TEST_ASSERT_EQUAL_INT(
      -1,
      packet_validate(&packet));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_packet_encode_empty_payload);
  RUN_TEST(test_packet_encode_one_byte_payload);
  RUN_TEST(test_packet_encode_payload_containing_sof);
  RUN_TEST(test_packet_encode_sequence_zero);
  RUN_TEST(test_packet_encode_sequence_255);
  RUN_TEST(test_packet_encode_type_zero);
  RUN_TEST(test_packet_encode_type_255);
  RUN_TEST(test_packet_encode_maximum_payload);

  RUN_TEST(test_packet_decode_known_packet);
  RUN_TEST(test_packet_decode_zero_length_payload);

  RUN_TEST(test_packet_validate_accepts_valid_packet);
  RUN_TEST(test_packet_validate_rejects_corrupted_payload);
  RUN_TEST(test_packet_validate_rejects_corrupted_sequence);
  RUN_TEST(test_packet_validate_rejects_corrupted_type);
  RUN_TEST(test_packet_validate_rejects_corrupted_crc);

  return UNITY_END();
}
