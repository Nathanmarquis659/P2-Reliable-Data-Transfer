#ifndef RX_H
#define RX_H

#include <stddef.h>
#include <stdint.h>
#include "pkt.h"

#define RX_IDLE_GIVEUP_MS 30000
#define RX_LINGER_MS 2000

typedef enum {
    RX_ACT_NONE = 0,
    RX_ACT_ACK,
    RX_ACT_WRITE_ACK,
    RX_ACT_DONE,
    RX_ACT_GIVEUP
} rx_action_t;

typedef struct {
    uint32_t expected;
    int finished;
    long linger_until_ms;
    long last_valid_ms;
    const uint8_t *payload;
    size_t payload_len;
} rx_state_t;

void rx_init(rx_state_t *st, long now_ms);

rx_action_t rx_on_packet(rx_state_t *st, long now_ms, const uint8_t *buf, size_t len);

rx_action_t rx_tick(rx_state_t *st, long now_ms);

#endif // RX_H
