//decode packets
//sequence tracking
//resynchronization

#include <stdint.h>
#include <stddef.h>
#include "../include/packet.h"

enum parser_state {
  SEARCH_SOF,
  READ_HEADER,
  READ_PAYLOAD,
  READ_CRC,
};

typedef void (*packet_callback)(struct packet *packet);

struct parser {
  enum parser_state state;
  packet_callback callback;

  uint8_t frame_buf[261];
  size_t frame_len;
};

void parser_init(struct parser *parser, packet_callback callback) {
  parser->state = SEARCH_SOF;
  parser->callback = callback;
  parser->frame_len = 0;
}

void parser_consume_byte(struct parser *parser, uint8_t byte) {
  switch (parser->state) {
    case SEARCH_SOF:
      if(byte == SOF) {
        parser->frame_buf[0] = byte;
        parser->frame_len = 1;
        parser->state = READ_HEADER;
      }
      break;

    case READ_HEADER:
      break;

    case READ_PAYLOAD:
      break;

    case READ_CRC:
      break;
  }
}

void parser_feed(struct parser *parser, uint8_t *data, size_t len) {
  for(size_t i = 0; i < len; i++) {
    parser_consume_byte(parser, data[i]);
  } }
