#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include "../include/crc.h"

#define SOF 0x2b
#define SOF_SIZE 1
#define MIN_PACKET_SIZE 6
#define MAX_PACKET_SIZE 261
#define PACKET_HEADER_SIZE 3
#define PACKET_CRC_SIZE 2


struct packet {
  uint8_t len;
  uint8_t seq;
  uint8_t type;
  uint8_t *payload;
  uint16_t crc;
};

struct packet *packet_create(uint8_t seq, uint8_t type, uint8_t *payload, uint8_t len);
size_t packet_encode(struct packet *packet, uint8_t *buf, size_t buf_cap);
int packet_encode_crc(struct packet *packet);
struct packet *packet_decode(uint8_t *buf, size_t buf_len);
int packet_cmp(struct packet *lhs, struct packet *rhs);
int packet_validate(struct packet *packet);
void packet_free(struct packet *packet);

#endif // !PROTOCOL_H
