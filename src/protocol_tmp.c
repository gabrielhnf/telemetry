//encode packets
//decode packets
//CRC
//sequence tracking
//resynchronization

#include <asm-generic/errno.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_PACKET_SIZE 6
#define MAX_PACKET_SIZE 261

uint16_t crc16arc(uint8_t *data, size_t len) {
  uint16_t crc = 0;
  uint16_t poly = 0xA001;

  for(int i = 0; i < len; i++) {
    crc ^= (uint16_t) data[i];

    for(int bit = 0; bit < 8; bit++) {
      if(crc & 1) {
        crc = (crc >> 1) ^ poly;
      } else {
        crc = (crc >> 1);
      }
    }
  }

  return crc;
}

struct packet {
  uint8_t len;
  uint8_t seq;
  uint8_t type;
  uint8_t *payload;
  uint16_t crc;
};

//|SOF|payload_size|seq_number|packet_type|payload|checksum|
//| 1 |     1      |    1     |     1     |   x   |   2    |

//TODO: Figure how to break this
size_t packet_encode(struct packet *packet, uint8_t *buf, size_t buf_cap) {
  if(buf_cap < MIN_PACKET_SIZE || buf_cap < packet->len + MIN_PACKET_SIZE) { //Design decision: what should be done when buffer is smaller than packet?
    errno = ENOBUFS;
    return 0; //return 0 and not emit packet? emit a malformed or incomplete packet?
  }
  buf[0] = 0x2b;
  buf[1] = packet->len;
  buf[2] = packet->seq;
  buf[3] = packet->type;

  for(int i = 0; i < packet->len; i++) {
    buf[4 + i] = packet->payload[i];
  }

  uint16_t crc = crc16arc(packet->payload, packet->len);
  buf[4 + packet->len] = (uint8_t)(crc >> 8);
  buf[5 + packet->len] = (uint8_t)(crc);

  //What can go wrong?

  return (6 + packet->len); //Isn't this too naive?

}

int main() {
  uint8_t *buf = malloc(261);

  char *payload = "123456789";
  struct packet packet = {strlen(payload), 0, 0, (uint8_t *) payload, 0};

  size_t res = packet_encode(&packet, buf, 261);

  printf("%ld bytes written.\n", res);
  for(int i = 0; i < res; i++) {
    printf("%#04x ", buf[i]);
  }

}
