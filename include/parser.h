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

  uint8_t frame_buf[MAX_PACKET_SIZE];
  size_t frame_len;
};

void parser_init(struct parser *parser, packet_callback callback);
void parser_consume_byte(struct parser *parser, uint8_t byte);
void parser_feed(struct parser *parser, uint8_t *data, size_t len);
void parser_resync(struct parser *parser);
