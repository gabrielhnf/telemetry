//decode packets
//sequence tracking
//resynchronization
//
//make sure a packet can be split across arbitrary parser_feed() calls;
//feed one byte at a time;
//feed several packets in one call;
//put garbage before/between packets;
//corrupt payload bytes;
//corrupt header bytes;
//corrupt CRC;
//insert fake 0x2B SOFs;
//truncate packets;
//make sure it resynchronizes and eventually finds the next valid packet;
//run with AddressSanitizer/UBSan and make sure malformed input never causes out-of-bounds access or UB.

#include "../include/parser.h"
#include "../include/packet.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void parser_init(struct parser *parser, packet_callback callback) {
  parser->state = SEARCH_SOF;
  parser->callback = callback;
  parser->frame_len = 0;
}

void parser_reset(struct parser *parser) {
  parser->state = SEARCH_SOF;
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
      if(parser->frame_len >= SOF_SIZE + PACKET_HEADER_SIZE) parser->state = READ_PAYLOAD;
      break;

    case READ_PAYLOAD:
      parser->frame_buf[parser->frame_len++] = byte;
      if(parser->frame_len - (PACKET_HEADER_SIZE + SOF_SIZE) >= parser->frame_buf[1])
        parser->state = READ_CRC;
      break;

    case READ_CRC:
      parser->frame_buf[parser->frame_len++] = byte;
      if(parser->frame_len >= parser->frame_buf[1] + MIN_PACKET_SIZE) {
        struct packet *p = packet_decode(parser->frame_buf, parser->frame_len);
        if(packet_validate(p) == -1) {
          packet_free(p);
          parser_resync(parser);
        } else {
          parser->callback(p);
          packet_free(p);
          parser_reset(parser);
        }
      }
      break;
  }
}

void parser_resync(struct parser *parser) {
  size_t next_sof = 0;
  int i = 1;

  while(i < parser->frame_len && next_sof == 0) {
    if(parser->frame_buf[i] == SOF) next_sof = i;
    i++;
  }

  if(next_sof == 0) {
    parser_reset(parser);
    return;
  };

  size_t remaining = parser->frame_len - next_sof;
  memmove(parser->frame_buf, parser->frame_buf + next_sof, remaining);

  parser->frame_len = remaining;

  for(size_t j = 0; j < parser->frame_len; j++) {
    parser_consume_byte(parser, parser->frame_buf[j]);
  }
  
}

void parser_feed(struct parser *parser, uint8_t *data, size_t len) {
  for(size_t i = 0; i < len; i++) parser_consume_byte(parser, data[i]);
}
