#include "pkt.h"
#include <string.h>

uint16_t pkt_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    size_t i = 0;
    for (; i + 1 < len; i += 2)
        sum += ((uint32_t)data[i] << 8) | (uint32_t)data[i + 1];
    if (i < len)
        sum += (uint32_t)data[i] << 8;
    while (sum >> 16)
        sum = (sum & 0xffffu) + (sum >> 16);
    return (uint16_t)(~sum & 0xffffu);
}

int pkt_encode(uint8_t *buf, size_t cap, int type, uint32_t seq,
               const uint8_t *payload, uint16_t payload_len)
{
    if (type < PKT_TYPE_DATA || type > PKT_TYPE_FIN)
        return -1;
    if (payload_len > PKT_MAX_PAYLOAD)
        return -1;
    size_t total = (size_t)PKT_HEADER_LEN + payload_len;
    if (cap < total)
        return -1;

    buf[0] = (uint8_t)type;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = (uint8_t)(seq >> 24);
    buf[5] = (uint8_t)(seq >> 16);
    buf[6] = (uint8_t)(seq >> 8);
    buf[7] = (uint8_t)seq;
    buf[8] = (uint8_t)(payload_len >> 8);
    buf[9] = (uint8_t)payload_len;
    if (payload_len != 0)
        memcpy(buf + PKT_HEADER_LEN, payload, payload_len);

    uint16_t csum = pkt_checksum(buf, total);
    buf[2] = (uint8_t)(csum >> 8);
    buf[3] = (uint8_t)csum;
    return (int)total;
}

int pkt_validate(const uint8_t *buf, size_t len, pkt_fields_t *out)
{
    if (len < PKT_HEADER_LEN)
        return -1;
    uint16_t length = (uint16_t)(((uint16_t)buf[8] << 8) | (uint16_t)buf[9]);
    if (length > PKT_MAX_PAYLOAD)
        return -1;
    if ((size_t)PKT_HEADER_LEN + length != len)
        return -1;
    int type = buf[0];
    if (type < PKT_TYPE_DATA || type > PKT_TYPE_FIN)
        return -1;
    if (buf[1] != 0)
        return -1;
    if (pkt_checksum(buf, len) != 0)
        return -1;

    out->type = type;
    out->seq = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) |
               ((uint32_t)buf[6] << 8) | (uint32_t)buf[7];
    out->length = length;
    return 0;
}
