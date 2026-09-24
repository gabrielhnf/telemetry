#include "unity/unity.h"
#include "unity/unity_internals.h"
#include "../include/parser.h"
#include <stdio.h>

void on_packet_echo(struct packet *p) {
  printf("Packet Info:\n");
  printf("Length = %d\n", p->len);
  printf("Sequence Number = %d\n", p->seq);
  printf("Type = %d\n", p->type);
  printf("Payload = ");
  for(int i = 0; i < p->len; i++) {
    printf("0x%02X ", p->payload[i]);
  }
  printf("\n");
  printf("CRC = 0x%04X\n", p->crc);
  printf("----------\n");
}

void setUp(void) {}
void tearDown(void) {}

void test_parser(void) {
  struct parser parser = {0};
  uint8_t buffer[] = { 
    0x01, 0x02, 0x03, 0x04, //trash
    0x2B, 0x02, 0xFF, 0xFF, 0x01, 0x02, 0xB5, 0xF9, //valid packet
    0x05, 0x06, 0x07, 0x08  //trash
  };
  size_t buffer_size = sizeof(buffer)/sizeof(uint8_t);

  printf("buffer: ");
  for(int i = 0; i < buffer_size; i++) {
    printf("0x%02X ", buffer[i]);
  }
  printf("\n");

  parser_init(&parser, on_packet_echo);
  parser_feed(&parser, buffer, buffer_size);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_parser);

  return UNITY_END();
}
