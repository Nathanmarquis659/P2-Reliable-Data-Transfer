#ifndef PKT_H
#define PKT_H

#include <stddef.h>
#include <stdint.h>

enum {
    PKT_TYPE_DATA = 0,
    PKT_TYPE_ACK = 1,
    PKT_TYPE_FIN = 2
};

#define PKT_HEADER_LEN 10
#define PKT_MAX_PAYLOAD 1024
#define PKT_MAX_SIZE (PKT_HEADER_LEN + PKT_MAX_PAYLOAD)

typedef struct {
    int type;
    uint32_t seq;
    uint16_t length;
} pkt_fields_t;

uint16_t pkt_checksum(const uint8_t *data, size_t len);

int pkt_encode(uint8_t *buf, size_t cap, int type, uint32_t seq,
               const uint8_t *payload, uint16_t payload_len);

int pkt_validate(const uint8_t *buf, size_t len, pkt_fields_t *out);

#endif // PKT_H
