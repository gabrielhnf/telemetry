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
      parser->frame_buf[parser->frame_len++] = byte;
      if(parser->frame_len > SOF_SIZE + PACKET_HEADER_SIZE) parser->state = READ_PAYLOAD;
      break;

    case READ_PAYLOAD:
      if(parser->frame_len - (PACKET_HEADER_SIZE + SOF_SIZE) < parser->frame_buf[1])
        parser->frame_buf[parser->frame_len++] = byte;
      else parser->state = READ_CRC;
      break;

    case READ_CRC:
      if(parser->frame_len < parser->frame_buf[1] + PACKET_HEADER_SIZE + SOF_SIZE + PACKET_CRC_SIZE)
        parser->frame_buf[parser->frame_len++] = byte;
      else {
        //decode packet
        //call callback
        parser->callback(NULL); 
        //should the callback return crc validation?
        //or perform it here?
        //in any case, in the event of invalid CRC, perform resynchronization
        parser->state = SEARCH_SOF;
      }
      break;
  }
}

void parser_feed(struct parser *parser, uint8_t *data, size_t len) {
  for(size_t i = 0; i < len; i++) {
    parser_consume_byte(parser, data[i]);
  } }
