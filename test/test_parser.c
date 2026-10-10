#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "unity/unity.h"
#include "../include/parser.h"
#include "../include/packet.h"

#define MAX_PAYLOAD_SIZE 255
#define MAX_FRAME_SIZE 261
#define MAX_CAPTURED_PACKETS 16

struct captured_packet {
  uint8_t len;
  uint8_t seq;
  uint8_t type;
  uint8_t payload[MAX_PAYLOAD_SIZE];
  uint16_t crc;
};

static struct captured_packet captured[MAX_CAPTURED_PACKETS];
static size_t captured_count;

/*
 * Test-side CRC-16/ARC implementation.
 *
 * This is intentionally kept inside the test file so arbitrary valid
 * frames can be constructed without depending on the production packet
 * encoder.
 */
static uint16_t test_crc16_arc(const uint8_t *data, size_t len) {
  uint16_t crc = 0x0000;

  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];

    for (int bit = 0; bit < 8; bit++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }

  return crc;
}

/*
 * Builds:
 *
 * | SOF | LEN | SEQ | TYPE | PAYLOAD | CRC16 |
 *
 * CRC covers:
 *
 * LEN | SEQ | TYPE | PAYLOAD
 *
 * CRC is emitted little-endian.
 */
static size_t build_frame(
    uint8_t *out,
    uint8_t seq,
    uint8_t type,
    const uint8_t *payload,
    uint8_t len) {
  size_t pos = 0;

  out[pos++] = SOF;
  out[pos++] = len;
  out[pos++] = seq;
  out[pos++] = type;

  if (len > 0) {
    memcpy(&out[pos], payload, len);
    pos += len;
  }

  uint16_t crc = test_crc16_arc(&out[1], 3 + len);

  out[pos++] = (uint8_t)((crc >> 8) & 0xFF); // high byte first
  out[pos++] = (uint8_t)(crc & 0xFF);        // low byte second

  return pos;
}

static void capture_packet(struct packet *pkt) {
  TEST_ASSERT_LESS_THAN(MAX_CAPTURED_PACKETS, captured_count);

  struct captured_packet *dst = &captured[captured_count++];

  dst->len = pkt->len;
  dst->seq = pkt->seq;
  dst->type = pkt->type;
  dst->crc = pkt->crc;

  if (pkt->len > 0) {
    TEST_ASSERT_NOT_NULL(pkt->payload);
    memcpy(dst->payload, pkt->payload, pkt->len);
  }
}

void setUp(void) {
  memset(captured, 0, sizeof(captured));
  captured_count = 0;
}

void tearDown(void) {
}

void test_parser_finds_packet_between_garbage(void) {
  struct parser parser = {0};

  /*
   * This is the known-good packet you were already using:
   *
   * LEN     = 2
   * SEQ     = 0xFF
   * TYPE    = 0xFF
   * PAYLOAD = 01 02
   * CRC     = F9B5 on the wire as B5 F9
   */
  uint8_t stream[] = {
    0x01, 0x02, 0x03, 0x04,

    0x2B, 0x02, 0xFF, 0xFF,
    0x01, 0x02,
    0xB5, 0xF9,

    0x05, 0x06, 0x07, 0x08
  };

  parser_init(&parser, capture_packet);
  parser_feed(&parser, stream, sizeof(stream));

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x02, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0xFF, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0xFF, captured[0].type);

  TEST_ASSERT_EQUAL_HEX8(0x01, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0x02, captured[0].payload[1]);
}

void test_parser_resyncs_after_bad_crc(void) {
  struct parser parser = {0};

  uint8_t bad_payload[] = {
    0xAA,
    0xBB
  };

  uint8_t good_payload[] = {
    0x01,
    0x02
  };

  uint8_t bad_frame[MAX_FRAME_SIZE];
  uint8_t good_frame[MAX_FRAME_SIZE];

  size_t bad_len = build_frame(
      bad_frame,
      0x10,
      0x20,
      bad_payload,
      sizeof(bad_payload));

  size_t good_len = build_frame(
      good_frame,
      0x42,
      0x30,
      good_payload,
      sizeof(good_payload));

  /*
   * Deliberately corrupt the CRC.
   *
   * A single-bit corruption must cause this frame to fail CRC.
   */
  bad_frame[bad_len - 1] ^= 0x01;

  uint8_t stream[MAX_FRAME_SIZE * 2];

  memcpy(stream, bad_frame, bad_len);
  memcpy(stream + bad_len, good_frame, good_len);

  parser_init(&parser, capture_packet);
  parser_feed(&parser, stream, bad_len + good_len);

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x42, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x30, captured[0].type);
  TEST_ASSERT_EQUAL_HEX8(0x01, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0x02, captured[0].payload[1]);
}

void test_parser_recovers_packet_nested_inside_failed_candidate(void) {
  struct parser parser = {0};

  /*
   * Construct a completely valid inner packet.
   */
  uint8_t inner_payload[] = {
    0x11,
    0x22
  };

  uint8_t inner_frame[MAX_FRAME_SIZE];

  size_t inner_len = build_frame(
      inner_frame,
      0x55,
      0x66,
      inner_payload,
      sizeof(inner_payload));

  /*
   * Construct an outer packet whose payload contains the ENTIRE
   * valid inner frame.
   *
   * The two trailing bytes are just additional outer payload.
   */
  uint8_t outer_payload[MAX_PAYLOAD_SIZE];

  memcpy(outer_payload, inner_frame, inner_len);

  outer_payload[inner_len] = 0xAA;
  outer_payload[inner_len + 1] = 0xBB;

  uint8_t outer_frame[MAX_FRAME_SIZE];

  size_t outer_len = build_frame(
      outer_frame,
      0x10,
      0x20,
      outer_payload,
      (uint8_t)(inner_len + 2));

  /*
   * Make the OUTER frame invalid without touching the embedded
   * inner packet.
   */
  outer_frame[outer_len - 1] ^= 0x01;

  parser_init(&parser, capture_packet);
  parser_feed(&parser, outer_frame, outer_len);

  /*
   * The parser should:
   *
   * 1. Try outer SOF.
   * 2. Fail outer CRC.
   * 3. Search buffered bytes from outer SOF + 1.
   * 4. Find inner SOF.
   * 5. Validate inner packet.
   * 6. Emit exactly the inner packet.
   */
  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x55, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x66, captured[0].type);

  TEST_ASSERT_EQUAL_UINT8(2, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0x11, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0x22, captured[0].payload[1]);
}

void test_parser_skips_multiple_bad_candidates(void) {
  struct parser parser = {0};

  uint8_t payload_a[] = {
    0x10
  };

  uint8_t payload_b[] = {
    0x20,
    0x21
  };

  uint8_t payload_good[] = {
    0x30,
    0x31,
    0x32
  };

  uint8_t frame_a[MAX_FRAME_SIZE];
  uint8_t frame_b[MAX_FRAME_SIZE];
  uint8_t frame_good[MAX_FRAME_SIZE];

  size_t len_a = build_frame(
      frame_a,
      0x01,
      0x10,
      payload_a,
      sizeof(payload_a));

  size_t len_b = build_frame(
      frame_b,
      0x02,
      0x11,
      payload_b,
      sizeof(payload_b));

  size_t len_good = build_frame(
      frame_good,
      0x03,
      0x12,
      payload_good,
      sizeof(payload_good));

  frame_a[len_a - 1] ^= 0x01;
  frame_b[len_b - 1] ^= 0x01;

  uint8_t stream[MAX_FRAME_SIZE * 3];
  size_t pos = 0;

  memcpy(&stream[pos], frame_a, len_a);
  pos += len_a;

  memcpy(&stream[pos], frame_b, len_b);
  pos += len_b;

  memcpy(&stream[pos], frame_good, len_good);
  pos += len_good;

  parser_init(&parser, capture_packet);
  parser_feed(&parser, stream, pos);

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x03, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x12, captured[0].type);

  TEST_ASSERT_EQUAL_UINT8(3, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0x30, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0x31, captured[0].payload[1]);
  TEST_ASSERT_EQUAL_HEX8(0x32, captured[0].payload[2]);
}

void test_parser_does_not_resync_on_sof_inside_valid_payload(void) {
  struct parser parser = {0};

  /*
   * A valid packet is allowed to contain 0x2B in its payload.
   *
   * The parser must NOT treat it as a new frame while the current
   * candidate is still valid.
   */
  uint8_t payload[] = {
    0xAA,
    SOF,
    0xBB
  };

  uint8_t frame[MAX_FRAME_SIZE];

  size_t frame_len = build_frame(
      frame,
      0x15,
      0x25,
      payload,
      sizeof(payload));

  parser_init(&parser, capture_packet);
  parser_feed(&parser, frame, frame_len);

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x15, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x25, captured[0].type);

  TEST_ASSERT_EQUAL_UINT8(3, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0xAA, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(SOF, captured[0].payload[1]);
  TEST_ASSERT_EQUAL_HEX8(0xBB, captured[0].payload[2]);
}

void test_parser_preserves_partial_packet_between_feed_calls(void) {
  struct parser parser = {0};

  uint8_t bad_payload[] = {
    0xAA,
    0xBB
  };

  uint8_t good_payload[] = {
    0x41,
    0x42,
    0x43
  };

  uint8_t bad_frame[MAX_FRAME_SIZE];
  uint8_t good_frame[MAX_FRAME_SIZE];

  size_t bad_len = build_frame(
      bad_frame,
      0x01,
      0x10,
      bad_payload,
      sizeof(bad_payload));

  size_t good_len = build_frame(
      good_frame,
      0x02,
      0x20,
      good_payload,
      sizeof(good_payload));

  bad_frame[bad_len - 1] ^= 0x01;

  /*
   * Feed all of the invalid frame, followed by only PART of the
   * next valid frame.
   */
  size_t first_part_of_good = 4;

  uint8_t first_feed[MAX_FRAME_SIZE * 2];
  size_t first_feed_len = 0;

  memcpy(
      &first_feed[first_feed_len],
      bad_frame,
      bad_len);

  first_feed_len += bad_len;

  memcpy(
      &first_feed[first_feed_len],
      good_frame,
      first_part_of_good);

  first_feed_len += first_part_of_good;

  parser_init(&parser, capture_packet);

  parser_feed(
      &parser,
      first_feed,
      first_feed_len);

  /*
   * The bad frame must not be emitted, and the valid frame is not
   * complete yet.
   */
  TEST_ASSERT_EQUAL_UINT32(0, captured_count);

  /*
   * Feed the remainder of the valid packet.
   */
  parser_feed(
      &parser,
      &good_frame[first_part_of_good],
      good_len - first_part_of_good);

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x02, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x20, captured[0].type);

  TEST_ASSERT_EQUAL_UINT8(3, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0x41, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0x42, captured[0].payload[1]);
  TEST_ASSERT_EQUAL_HEX8(0x43, captured[0].payload[2]);
}

void test_parser_accepts_packet_one_byte_at_a_time(void) {
  struct parser parser = {0};

  uint8_t payload[] = {
    0xDE,
    0xAD,
    0xBE,
    0xEF
  };

  uint8_t frame[MAX_FRAME_SIZE];

  size_t frame_len = build_frame(
      frame,
      0x73,
      0x42,
      payload,
      sizeof(payload));

  parser_init(&parser, capture_packet);

  for (size_t i = 0; i < frame_len; i++) {
    parser_feed(&parser, &frame[i], 1);
  }

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x73, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x42, captured[0].type);

  TEST_ASSERT_EQUAL_UINT8(4, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0xDE, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0xAD, captured[0].payload[1]);
  TEST_ASSERT_EQUAL_HEX8(0xBE, captured[0].payload[2]);
  TEST_ASSERT_EQUAL_HEX8(0xEF, captured[0].payload[3]);
}

void test_parser_emits_multiple_valid_packets(void) {
  struct parser parser = {0};

  uint8_t payload_a[] = {
    0xA1
  };

  uint8_t payload_b[] = {
    0xB1,
    0xB2
  };

  uint8_t frame_a[MAX_FRAME_SIZE];
  uint8_t frame_b[MAX_FRAME_SIZE];

  size_t len_a = build_frame(
      frame_a,
      0x10,
      0x20,
      payload_a,
      sizeof(payload_a));

  size_t len_b = build_frame(
      frame_b,
      0x11,
      0x21,
      payload_b,
      sizeof(payload_b));

  uint8_t stream[MAX_FRAME_SIZE * 2];
  size_t stream_len = 0;

  memcpy(&stream[stream_len], frame_a, len_a);
  stream_len += len_a;

  memcpy(&stream[stream_len], frame_b, len_b);
  stream_len += len_b;

  parser_init(&parser, capture_packet);
  parser_feed(&parser, stream, stream_len);

  TEST_ASSERT_EQUAL_UINT32(2, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x10, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x20, captured[0].type);
  TEST_ASSERT_EQUAL_HEX8(0xA1, captured[0].payload[0]);

  TEST_ASSERT_EQUAL_HEX8(0x11, captured[1].seq);
  TEST_ASSERT_EQUAL_HEX8(0x21, captured[1].type);
  TEST_ASSERT_EQUAL_HEX8(0xB1, captured[1].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0xB2, captured[1].payload[1]);
}

void test_parser_accepts_zero_length_payload(void) {
  struct parser parser = {0};

  uint8_t frame[MAX_FRAME_SIZE];

  size_t frame_len = build_frame(
      frame,
      0x10,
      0x20,
      NULL,
      0);

  parser_init(&parser, capture_packet);
  parser_feed(&parser, frame, frame_len);

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_UINT8(0, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0x10, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x20, captured[0].type);
}

void test_parser_accepts_maximum_payload(void) {
  struct parser parser = {0};

  uint8_t payload[255];

  for(size_t i = 0; i < sizeof(payload); i++) {
    payload[i] = (uint8_t)i;
  }

  uint8_t frame[MAX_FRAME_SIZE];

  size_t frame_len = build_frame(
      frame,
      0x11,
      0x21,
      payload,
      255);

  TEST_ASSERT_EQUAL_UINT32(MAX_FRAME_SIZE, frame_len);

  parser_init(&parser, capture_packet);
  parser_feed(&parser, frame, frame_len);

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_UINT8(255, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0x11, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x21, captured[0].type);

  for(size_t i = 0; i < sizeof(payload); i++) {
    TEST_ASSERT_EQUAL_HEX8(payload[i], captured[0].payload[i]);
  }
}

void test_parser_rejects_corrupted_sequence_number(void) {
  struct parser parser = {0};

  uint8_t payload[] = {
    0xAA,
    0xBB
  };

  uint8_t frame[MAX_FRAME_SIZE];

  size_t frame_len = build_frame(
      frame,
      0x12,
      0x22,
      payload,
      sizeof(payload));

  /*
   * SEQ is at frame[2].
   *
   * Change it without recalculating CRC.
   */
  frame[2] ^= 0x01;

  parser_init(&parser, capture_packet);
  parser_feed(&parser, frame, frame_len);

  TEST_ASSERT_EQUAL_UINT32(0, captured_count);
}

void test_parser_rejects_corrupted_packet_type(void) {
  struct parser parser = {0};

  uint8_t payload[] = {
    0xAA,
    0xBB
  };

  uint8_t frame[MAX_FRAME_SIZE];

  size_t frame_len = build_frame(
      frame,
      0x13,
      0x23,
      payload,
      sizeof(payload));

  /*
   * TYPE is at frame[3].
   */
  frame[3] ^= 0x01;

  parser_init(&parser, capture_packet);
  parser_feed(&parser, frame, frame_len);

  TEST_ASSERT_EQUAL_UINT32(0, captured_count);
}

void test_parser_does_not_emit_truncated_packet(void) {
  struct parser parser = {0};

  uint8_t payload[] = {
    0x01,
    0x02,
    0x03,
    0x04
  };

  uint8_t frame[MAX_FRAME_SIZE];

  size_t frame_len = build_frame(
      frame,
      0x14,
      0x24,
      payload,
      sizeof(payload));

  parser_init(&parser, capture_packet);

  /*
   * Leave off the last three bytes.
   */
  parser_feed(&parser, frame, frame_len - 3);

  TEST_ASSERT_EQUAL_UINT32(0, captured_count);
}

void test_parser_ignores_garbage_only(void) {
  struct parser parser = {0};

  uint8_t garbage[1024];

  /*
   * Deliberately avoid SOF.
   */
  for(size_t i = 0; i < sizeof(garbage); i++) {
    garbage[i] = (uint8_t)((i % 0x2A) + 1);
  }

  parser_init(&parser, capture_packet);
  parser_feed(&parser, garbage, sizeof(garbage));

  TEST_ASSERT_EQUAL_UINT32(0, captured_count);
}

void test_parser_handles_sof_as_last_byte_of_feed(void) {
  struct parser parser = {0};

  uint8_t payload[] = {
    0x31,
    0x32,
    0x33
  };

  uint8_t frame[MAX_FRAME_SIZE];

  size_t frame_len = build_frame(
      frame,
      0x15,
      0x25,
      payload,
      sizeof(payload));

  parser_init(&parser, capture_packet);

  /*
   * Feed only the SOF first.
   */
  parser_feed(&parser, frame, 1);

  TEST_ASSERT_EQUAL_UINT32(0, captured_count);

  /*
   * Feed the rest later.
   */
  parser_feed(
      &parser,
      &frame[1],
      frame_len - 1);

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x15, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x25, captured[0].type);

  TEST_ASSERT_EQUAL_UINT8(3, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0x31, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0x32, captured[0].payload[1]);
  TEST_ASSERT_EQUAL_HEX8(0x33, captured[0].payload[2]);
}

void test_parser_handles_multiple_packets_split_across_feeds(void) {
  struct parser parser = {0};

  uint8_t payload_a[] = {
    0xA1,
    0xA2
  };

  uint8_t payload_b[] = {
    0xB1,
    0xB2,
    0xB3
  };

  uint8_t frame_a[MAX_FRAME_SIZE];
  uint8_t frame_b[MAX_FRAME_SIZE];

  size_t len_a = build_frame(
      frame_a,
      0x20,
      0x30,
      payload_a,
      sizeof(payload_a));

  size_t len_b = build_frame(
      frame_b,
      0x21,
      0x31,
      payload_b,
      sizeof(payload_b));

  uint8_t stream[MAX_FRAME_SIZE * 2];
  size_t stream_len = 0;

  memcpy(&stream[stream_len], frame_a, len_a);
  stream_len += len_a;

  memcpy(&stream[stream_len], frame_b, len_b);
  stream_len += len_b;

  /*
   * Split in the middle of packet B.
   */
  size_t split = len_a + 3;

  parser_init(&parser, capture_packet);

  parser_feed(
      &parser,
      stream,
      split);

  /*
   * Packet A should already have been emitted.
   * Packet B is incomplete.
   */
  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  parser_feed(
      &parser,
      &stream[split],
      stream_len - split);

  TEST_ASSERT_EQUAL_UINT32(2, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x20, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x30, captured[0].type);
  TEST_ASSERT_EQUAL_HEX8(0xA1, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0xA2, captured[0].payload[1]);

  TEST_ASSERT_EQUAL_HEX8(0x21, captured[1].seq);
  TEST_ASSERT_EQUAL_HEX8(0x31, captured[1].type);
  TEST_ASSERT_EQUAL_HEX8(0xB1, captured[1].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0xB2, captured[1].payload[1]);
  TEST_ASSERT_EQUAL_HEX8(0xB3, captured[1].payload[2]);
}

void test_parser_accepts_packet_at_every_possible_split(void) {
  uint8_t payload[] = {
    0xDE,
    0xAD,
    0xBE,
    0xEF,
    0x42
  };

  uint8_t frame[MAX_FRAME_SIZE];

  size_t frame_len = build_frame(
      frame,
      0x40,
      0x50,
      payload,
      sizeof(payload));

  /*
   * Try every possible place where read() could have split
   * this frame into two chunks.
   */
  for(size_t split = 1; split < frame_len; split++) {
    struct parser parser = {0};

    memset(captured, 0, sizeof(captured));
    captured_count = 0;

    parser_init(&parser, capture_packet);

    parser_feed(
        &parser,
        frame,
        split);

    TEST_ASSERT_EQUAL_UINT32(0, captured_count);

    parser_feed(
        &parser,
        &frame[split],
        frame_len - split);

    TEST_ASSERT_EQUAL_UINT32(1, captured_count);

    TEST_ASSERT_EQUAL_HEX8(0x40, captured[0].seq);
    TEST_ASSERT_EQUAL_HEX8(0x50, captured[0].type);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), captured[0].len);

    for(size_t i = 0; i < sizeof(payload); i++) {
      TEST_ASSERT_EQUAL_HEX8(
          payload[i],
          captured[0].payload[i]);
    }
  }
}

void test_parser_recovers_after_corrupted_length(void) {
  struct parser parser = {0};

  uint8_t good_payload[] = {
    0x61,
    0x62
  };

  uint8_t good_frame[MAX_FRAME_SIZE];

  size_t good_len = build_frame(
      good_frame,
      0x60,
      0x70,
      good_payload,
      sizeof(good_payload));

  /*
   * Build a false candidate which claims a 10-byte payload.
   *
   * The good packet is embedded inside those bytes. Once the
   * false candidate eventually fails CRC, resynchronization
   * should locate the embedded SOF and recover it.
   */
  uint8_t stream[MAX_FRAME_SIZE];
  size_t pos = 0;

  stream[pos++] = SOF;
  stream[pos++] = 10;
  stream[pos++] = 0x01;
  stream[pos++] = 0x02;

  /*
   * Embed the valid frame inside the claimed payload.
   */
  memcpy(&stream[pos], good_frame, good_len);
  pos += good_len;

  /*
   * Fill whatever remains of the false candidate's payload.
   *
   * good_len for a 2-byte payload is 8 bytes, so two bytes
   * remain to reach the claimed 10-byte payload.
   */
  stream[pos++] = 0xAA;
  stream[pos++] = 0xBB;

  /*
   * Invalid CRC for the false outer candidate.
   */
  stream[pos++] = 0x00;
  stream[pos++] = 0x00;

  parser_init(&parser, capture_packet);
  parser_feed(&parser, stream, pos);

  TEST_ASSERT_EQUAL_UINT32(1, captured_count);

  TEST_ASSERT_EQUAL_HEX8(0x60, captured[0].seq);
  TEST_ASSERT_EQUAL_HEX8(0x70, captured[0].type);
  TEST_ASSERT_EQUAL_UINT8(2, captured[0].len);
  TEST_ASSERT_EQUAL_HEX8(0x61, captured[0].payload[0]);
  TEST_ASSERT_EQUAL_HEX8(0x62, captured[0].payload[1]);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_parser_finds_packet_between_garbage);
  RUN_TEST(test_parser_resyncs_after_bad_crc);
  RUN_TEST(test_parser_recovers_packet_nested_inside_failed_candidate);
  RUN_TEST(test_parser_skips_multiple_bad_candidates);
  RUN_TEST(test_parser_does_not_resync_on_sof_inside_valid_payload);
  RUN_TEST(test_parser_preserves_partial_packet_between_feed_calls);
  RUN_TEST(test_parser_accepts_packet_one_byte_at_a_time);
  RUN_TEST(test_parser_emits_multiple_valid_packets);
  RUN_TEST(test_parser_accepts_zero_length_payload);
  RUN_TEST(test_parser_accepts_maximum_payload);
  RUN_TEST(test_parser_rejects_corrupted_sequence_number);
  RUN_TEST(test_parser_rejects_corrupted_packet_type);
  RUN_TEST(test_parser_does_not_emit_truncated_packet);
  RUN_TEST(test_parser_ignores_garbage_only);
  RUN_TEST(test_parser_handles_sof_as_last_byte_of_feed);
  RUN_TEST(test_parser_handles_multiple_packets_split_across_feeds);
  RUN_TEST(test_parser_accepts_packet_at_every_possible_split);
  RUN_TEST(test_parser_recovers_after_corrupted_length);

  return UNITY_END();
}
