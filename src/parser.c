//decode packets
//sequence tracking
//resynchronization

#include "../include/parser.h"
#include "../include/packet.h"
#include <stdio.h>

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
      if(parser->frame_len >= SOF_SIZE + PACKET_HEADER_SIZE) parser->state = READ_PAYLOAD;
      break;

    case READ_PAYLOAD:
      parser->frame_buf[parser->frame_len++] = byte;
      if(parser->frame_len - (PACKET_HEADER_SIZE + SOF_SIZE) < parser->frame_buf[1])
        parser->state = READ_CRC;
      break;

    case READ_CRC:
      parser->frame_buf[parser->frame_len++] = byte;
      if(parser->frame_len >= parser->frame_buf[1] + MIN_PACKET_SIZE) {
        printf("Parser buffer: ");
        for(int i = 0; i < parser->frame_len; i++) {
          printf("0x%02X ", parser->frame_buf[i]);
        }
        printf("\n");
        struct packet *p = packet_decode(parser->frame_buf, parser->frame_len);
        parser->callback(p); 
        //crc validation occurs here, returns if need resync (dropped packet) or not
        parser->state = SEARCH_SOF;
      }
      break;
  }
}

void parser_feed(struct parser *parser, uint8_t *data, size_t len) {
  for(size_t i = 0; i < len; i++) {
    parser_consume_byte(parser, data[i]);
  } }
