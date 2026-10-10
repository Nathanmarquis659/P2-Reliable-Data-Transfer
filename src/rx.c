#include "rx.h"

void rx_init(rx_state_t *st, long now_ms)
{
    st->expected = 0;
    st->finished = 0;
    st->linger_until_ms = 0;
    st->last_valid_ms = now_ms;
    st->payload = NULL;
    st->payload_len = 0;
}

rx_action_t rx_on_packet(rx_state_t *st, long now_ms, const uint8_t *buf, size_t len)
{
    pkt_fields_t f;
    if (pkt_validate(buf, len, &f) != 0)
        return RX_ACT_NONE;

    st->last_valid_ms = now_ms;

    if (st->finished)
        return RX_ACT_ACK;

    if (f.type == PKT_TYPE_DATA) {
        if (f.seq != st->expected)
            return RX_ACT_ACK;
        st->payload = buf + PKT_HEADER_LEN;
        st->payload_len = f.length;
        st->expected += 1;
        return RX_ACT_WRITE_ACK;
    }

    if (f.type == PKT_TYPE_FIN) {
        if (f.seq != st->expected)
            return RX_ACT_ACK;
        st->finished = 1;
        st->expected += 1;
        st->linger_until_ms = now_ms + RX_LINGER_MS;
        st->payload = NULL;
        st->payload_len = 0;
        return RX_ACT_ACK;
    }

    return RX_ACT_NONE;
}

rx_action_t rx_tick(rx_state_t *st, long now_ms)
{
    if (st->finished) {
        if (now_ms >= st->linger_until_ms)
            return RX_ACT_DONE;
        return RX_ACT_NONE;
    }
    if (now_ms - st->last_valid_ms >= RX_IDLE_GIVEUP_MS)
        return RX_ACT_GIVEUP;
    return RX_ACT_NONE;
}
