#ifndef SX_H
#define SX_H

#include <stddef.h>
#include <stdint.h>
#include "pkt.h"

#define SX_MAX_WINDOW 64
#define SX_GIVEUP_TIMEOUTS 10

typedef enum {
    SX_ST_ACTIVE = 0,
    SX_ST_DONE,
    SX_ST_GAVEUP
} sx_status_t;

typedef struct {
    const uint8_t *buf; /* valid until the next sx_* call */
    int len;
} sx_send_t;

typedef struct {
    uint8_t buf[PKT_MAX_SIZE];
    int len;
} sx_pkt_t;

typedef struct {
    int window;
    long timeout_ms;
    const uint8_t *file;
    size_t file_len;
    uint32_t n_data;
    uint32_t base;
    uint32_t next;
    sx_pkt_t w[SX_MAX_WINDOW];
    int retransmit;
    int rt_fin;
    uint32_t rt_base;
    uint32_t rt_next;
    uint32_t rt_cursor;
    int fin_sent;
    int fin_acked;
    sx_pkt_t fin_pkt;
    int timer_running;
    long deadline_ms;
    uint32_t last_base;
    int stuck_timeouts;
    int done;
    int gave_up;
} sx_state_t;

/* Precondition: 1 <= window <= SX_MAX_WINDOW (CLI-validated).
 * file is borrowed and must outlive the state. */
void sx_init(sx_state_t *st, int window, long timeout_ms, const uint8_t *file, size_t file_len);

void sx_on_packet(sx_state_t *st, long now_ms, const uint8_t *buf, size_t len);

void sx_tick(sx_state_t *st, long now_ms);

int sx_next_send(sx_state_t *st, long now_ms, sx_send_t *out);

sx_status_t sx_status(const sx_state_t *st);

long sx_timer_remaining_ms(const sx_state_t *st, long now_ms);

#endif // SX_H
