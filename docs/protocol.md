Packet:

|SOF|payload_size|seq_number|packet_type|payload|checksum|
| 1 |     1      |    1     |     1     |   x   |   2    |

SOF:
    size: 1 byte
    constant byte value: 0x2B

payload_size:
    size: 1 byte
    range: 0..255
    meaning: number of bytes in payload

seq_number:
    size: 1 byte
    range: 0..255
    wraparound: modulo 256
    independent per trasmitter

packet_type:
    size: 1 byte
    range: 0..255
    identifies message semantics

payload:
    size: payload_size bytes

checksum:
    size: 2 bytes
    spans:  seq_number, packet_type, payload, payload_size
    type: crc-16-arc
    polynomial: 0x8005
    initial_value: 0x0000
    ref_in: true
    ref_out: true
    xor_out: 0x0000
    byte_order: little endian
    on_fail: {
        discard the first byte of candidate frame,
        search remaining buffered bytes for SOF,
        if none exists {
            resume searching incoming stream bytes
        }
        for each candidate SOF {
            parse header,
            wait for payload_size bytes + CRC,
            validate CRC,
            if CRC is valid {
                process packet
            } else {
                repeat recovery from next candidate
            }
        }
    }

minimum frame size:
    6 bytes

maximum frame size:
    261 bytes
