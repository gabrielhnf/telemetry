//encode packets ✅
//CRC ✅

#include "../include/packet.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int packet_encode_crc(struct packet *packet) {
  uint8_t *buf = malloc(packet->len + 3);
  if(!buf) return -1;

  buf[0] = packet->len;
  buf[1] = packet->seq;
  buf[2] = packet->type;
  memcpy(buf + 3, packet->payload, packet->len);

  packet->crc = crc16arc(buf, packet->len + 3);
  free(buf);
  return 0;
}

//Should the packet own the payload? Or simply keep a reference to it? Do we care about this stuff in embedded programming?
//Should we even try encapsulation? Is it important in embedded programming?
struct packet *packet_create(uint8_t seq, uint8_t type, uint8_t *payload, uint8_t len) {
  if(!payload) {
    errno = EINVAL;
    return NULL;
  }

  struct packet *packet = malloc(sizeof(struct packet));
  if(!packet) return NULL;

  packet->len = len;
  packet->seq = seq;
  packet->type = type;

  uint8_t *payload_buf = malloc(len);
  if(!payload_buf) {
    free(packet);
    return NULL;
  }

  memcpy(payload_buf, payload, len);
  packet->payload = payload_buf;

  if(packet_encode_crc(packet) == -1) {
    free(packet->payload);
    free(packet);
    return NULL;
  }

  return packet;
}

void packet_free(struct packet *packet) {
  free(packet->payload);
  free(packet);
}

size_t packet_encode(struct packet *packet, uint8_t *buf, size_t buf_len) {
  if(!buf) {
    errno = EINVAL;
    return 0;
  }

  if(buf_len < (size_t)(packet->len + MIN_PACKET_SIZE)) {
    errno = ENOBUFS;
    return 0;
  }

  buf[0] = SOF;
  buf[1] = packet->len;
  buf[2] = packet->seq;
  buf[3] = packet->type;
  memcpy(buf + 4, packet->payload, packet->len);

  buf[4 + packet->len] = (uint8_t)(packet->crc >> 8);
  buf[5 + packet->len] = (uint8_t)(packet->crc);

  return (6 + packet->len);
}

//Should return a reference to a packet or the packet itself?
//Should we try a consumer pattern? (consume_sof(), consume_header(), etc)
struct packet *packet_decode(uint8_t *buf, size_t buf_len) { 
  struct packet *packet = malloc(sizeof(struct packet));
  if(!packet) return NULL;

  for(size_t i = 0; i < buf_len; i++) {
    uint8_t byte = buf[i];

    if(byte == SOF) {
      i++; //consumes SOF
      //seems a bit dangerous
      uint8_t len = buf[i++];
      uint8_t seq = buf[i++];
      uint8_t type = buf[i++];
      uint8_t *payload_buf = malloc(len); //can fail
      size_t start_index = i;
      for(; i - start_index < len; i++) //does this work?
        payload_buf[i - start_index] = buf[i];
      uint16_t crc = (buf[i++] << 8);
      crc ^= buf[i];

      packet->len = len;
      packet->seq = seq;
      packet->type = type;
      packet->payload = payload_buf;
      packet->crc = crc;

      return packet;

    }

  }
  return NULL;
}

int packet_cmp(struct packet *lhs, struct packet *rhs) {
  int res = 1;
  res &= lhs->len == rhs->len;
  res &= lhs->seq == rhs->seq;
  res &= lhs->type == rhs->type;
  for(int i = 0; i < lhs->len; i++) {
    res &= lhs->payload[i] == rhs->payload[i];
  }
  res &= lhs->crc == rhs->crc;
  return res;
}
