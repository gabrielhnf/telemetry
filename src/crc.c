#include <stddef.h>
#include <stdint.h>

uint16_t crc16_arc(const uint8_t *data, size_t len);

/*
 
    polynomial: 0x8005
    initial_value: 0x0000
    ref_in: true
    ref_out: true
    xor_out: 0x0000

 */

// "hi" => "01101000 01101001"
