#include "../include/crc.h"

uint16_t crc16arc(uint8_t *data, size_t len) {
  uint16_t crc = 0;
  uint16_t poly = 0xA001;

  for(size_t i = 0; i < len; i++) {
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
