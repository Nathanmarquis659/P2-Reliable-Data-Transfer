#include <string.h>

#include "sx.h"

static void sx_timer_start(sx_state_t *st, long now_ms)
{
    if (!st->timer_running) {
        st->timer_running = 1;
        st->deadline_ms = now_ms + st->timeout_ms;
    }
}

static void sx_timer_restart(sx_state_t *st, long now_ms)
{
    st->timer_running = 1;
    st->deadline_ms = now_ms + st->timeout_ms;
}

void sx_init(sx_state_t *st, int window, long timeout_ms, const uint8_t *file, size_t file_len)
{
    memset(st, 0, sizeof(*st));
    st->window = window;
    st->timeout_ms = timeout_ms;
    st->file = file;
    st->file_len = file_len;
    st->n_data = (uint32_t)((file_len + PKT_MAX_PAYLOAD - 1u) / PKT_MAX_PAYLOAD);
}

void sx_on_packet(sx_state_t *st, long now_ms, const uint8_t *buf, size_t len)
{
    pkt_fields_t f;
    if (pkt_validate(buf, len, &f) != 0)
        return;
    if (f.type != PKT_TYPE_ACK)
        return;
    if (st->done || st->gave_up)
        return;

    if (st->fin_sent && !st->fin_acked) {
        if (f.seq == st->n_data + 1u) {
            st->fin_acked = 1;
            st->done = 1;
            st->timer_running = 0;
        }
        return;
    }

    if (f.seq > st->base) {
        uint32_t seq = f.seq;
        if (seq > st->n_data)
            seq = st->n_data;
        st->base = seq;
        if (st->next > st->base)
            sx_timer_restart(st, now_ms);
        else
            st->timer_running = 0;
    }
}

void sx_tick(sx_state_t *st, long now_ms)
{
    if (st->done || st->gave_up)
        return;
    if (!st->timer_running || now_ms < st->deadline_ms)
        return;

    if (st->base != st->last_base) {
        st->last_base = st->base;
        st->stuck_timeouts = 0;
    }
    st->stuck_timeouts += 1;
    if (st->stuck_timeouts >= SX_GIVEUP_TIMEOUTS) {
        st->gave_up = 1;
        st->timer_running = 0;
        return;
    }

    st->retransmit = 1;
    if (st->fin_sent && !st->fin_acked) {
        st->rt_fin = 1;
    } else {
        st->rt_fin = 0;
        st->rt_base = st->base;
        st->rt_next = st->next;
        st->rt_cursor = st->base;
    }
    sx_timer_restart(st, now_ms);
}

int sx_next_send(sx_state_t *st, long now_ms, sx_send_t *out)
{
    if (st->done || st->gave_up)
        return 0;

    if (st->retransmit) {
        if (st->rt_fin) {
            st->retransmit = 0;
            out->buf = st->fin_pkt.buf;
            out->len = st->fin_pkt.len;
            return 1;
        }
        if (st->rt_cursor < st->rt_next) {
            uint32_t seq = st->rt_cursor++;
            sx_pkt_t *p = &st->w[seq % SX_MAX_WINDOW];
            out->buf = p->buf;
            out->len = p->len;
            return 1;
        }
        st->retransmit = 0;
    }

    if (!st->fin_sent && st->base == st->n_data) {
        int n = pkt_encode(st->fin_pkt.buf, PKT_MAX_SIZE, PKT_TYPE_FIN, st->n_data, NULL, 0);
        st->fin_pkt.len = n;
        st->fin_sent = 1;
        sx_timer_start(st, now_ms);
        out->buf = st->fin_pkt.buf;
        out->len = n;
        return 1;
    }

    if (st->next < st->n_data && st->next < st->base + (uint32_t)st->window) {
        uint32_t seq = st->next;
        size_t off = (size_t)seq * (size_t)PKT_MAX_PAYLOAD;
        size_t remaining = st->file_len - off;
        uint16_t plen = (uint16_t)(remaining > PKT_MAX_PAYLOAD ? PKT_MAX_PAYLOAD : remaining);
        sx_pkt_t *p = &st->w[seq % SX_MAX_WINDOW];
        int n = pkt_encode(p->buf, PKT_MAX_SIZE, PKT_TYPE_DATA, seq, st->file + off, plen);
        p->len = n;
        st->next = seq + 1u;
        sx_timer_start(st, now_ms);
        out->buf = p->buf;
        out->len = n;
        return 1;
    }

    return 0;
}

sx_status_t sx_status(const sx_state_t *st)
{
    if (st->done)
        return SX_ST_DONE;
    if (st->gave_up)
        return SX_ST_GAVEUP;
    return SX_ST_ACTIVE;
}

long sx_timer_remaining_ms(const sx_state_t *st, long now_ms)
{
    if (!st->timer_running || st->done || st->gave_up)
        return 0;
    long rem = st->deadline_ms - now_ms;
    return rem > 0 ? rem : 0;
}
