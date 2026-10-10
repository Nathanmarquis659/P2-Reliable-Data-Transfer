#include "lab.h"
#include "pkt.h"
#include "rx.h"
#include "sx.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#ifdef TEST
#define main main_exclude
#endif

/* Layer 3 (I/O glue): the only code in the program that touches sockets,
 * the clock, or files. Layers 1-2 (pkt, sx, rx) are pure and driven by the
 * "now" values and actions produced here. */

#define HANDSHAKE_ATTEMPTS 5
#define HANDSHAKE_WAIT_MS 1000
#define POLL_CAP_MS 60000
#define IO_BUF_SIZE 2048

static long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

/* Resolve <relay> and open the one UDP socket used for the entire run. */
static int open_relay(const char *host, int port, struct sockaddr_storage *addr_out, socklen_t *len_out)
{
    char portstr[8];
    snprintf(portstr, sizeof(portstr), "%d", port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *res = NULL;
    int err = getaddrinfo(host, portstr, &hints, &res);
    if (err != 0 || res == NULL) {
        fprintf(stderr, "error: cannot resolve relay '%s': %s\n", host,
                err != 0 ? gai_strerror(err) : "no results");
        return -1;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        fprintf(stderr, "error: socket failed: %s\n", strerror(errno));
        freeaddrinfo(res);
        return -1;
    }
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0 || fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0) {
        fprintf(stderr, "error: fcntl(O_NONBLOCK) failed: %s\n", strerror(errno));
        close(fd);
        freeaddrinfo(res);
        return -1;
    }
    memcpy(addr_out, res->ai_addr, res->ai_addrlen);
    *len_out = res->ai_addrlen;
    freeaddrinfo(res);
    return fd;
}

/* Registration handshake: send HELLO, wait up to 1 s for OK/ERR, resend on
 * silence, give up after HANDSHAKE_ATTEMPTS total attempts. */
static int handshake(int fd, const struct sockaddr_storage *addr, socklen_t alen,
                     const char *session, int is_send, double loss, double corrupt, double dup)
{
    char hello[96];
    if (is_send)
        snprintf(hello, sizeof(hello), "HELLO %s send %.6f %.6f %.6f", session, loss, corrupt, dup);
    else
        snprintf(hello, sizeof(hello), "HELLO %s recv", session);

    uint8_t reply[256];
    for (int attempt = 1; attempt <= HANDSHAKE_ATTEMPTS; attempt++) {
        if (sendto(fd, hello, strlen(hello), 0, (const struct sockaddr *)addr, alen) < 0) {
            fprintf(stderr, "error: sendto failed: %s\n", strerror(errno));
            return -1;
        }
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        if (poll(&pfd, 1, HANDSHAKE_WAIT_MS) <= 0)
            continue;
        ssize_t n = recvfrom(fd, reply, sizeof(reply), 0, NULL, NULL);
        if (n <= 0)
            continue;
        if (memcmp(reply, "OK", 2) == 0)
            return 0;
        if (memcmp(reply, "ERR", 3) == 0) {
            fprintf(stderr, "error: relay refused: %.*s\n", (int)n, reply);
            return -1;
        }
    }
    fprintf(stderr, "error: no relay reply after %d attempts\n", HANDSHAKE_ATTEMPTS);
    return -1;
}

static int run_send(const config_t *cfg)
{
    FILE *f = fopen(cfg->file, "rb");
    if (f == NULL) {
        fprintf(stderr, "error: cannot open '%s': %s\n", cfg->file, strerror(errno));
        return 2;
    }
    long sz = -1;
    if (fseek(f, 0, SEEK_END) == 0)
        sz = ftell(f);
    if (sz < 0) {
        fprintf(stderr, "error: cannot determine size of '%s'\n", cfg->file);
        fclose(f);
        return 2;
    }
    rewind(f);
    uint8_t *file = malloc((size_t)(sz > 0 ? sz : 1));
    if (file == NULL) {
        fprintf(stderr, "error: out of memory\n");
        fclose(f);
        return 2;
    }
    if (sz > 0 && fread(file, 1, (size_t)sz, f) != (size_t)sz) {
        fprintf(stderr, "error: short read from '%s'\n", cfg->file);
        fclose(f);
        free(file);
        return 2;
    }
    fclose(f);

    struct sockaddr_storage addr;
    socklen_t alen = 0;
    int fd = open_relay(cfg->relay, cfg->port, &addr, &alen);
    if (fd < 0) {
        free(file);
        return 2;
    }
    if (handshake(fd, &addr, alen, cfg->session, 1, cfg->loss, cfg->corrupt, cfg->dup) != 0) {
        close(fd);
        free(file);
        return 2;
    }

    sx_state_t st;
    sx_init(&st, cfg->window, cfg->timeout_ms, file, (size_t)sz);

    uint8_t rbuf[IO_BUF_SIZE];
    int rc = 2;
    for (;;) {
        long now = now_ms();
        sx_send_t s;
        while (sx_next_send(&st, now, &s) == 1) {
            if (sendto(fd, s.buf, (size_t)s.len, 0, (const struct sockaddr *)&addr, alen) < 0) {
                fprintf(stderr, "error: sendto failed: %s\n", strerror(errno));
                goto done;
            }
        }

        sx_status_t status = sx_status(&st);
        if (status == SX_ST_DONE) {
            rc = 0;
            break;
        }
        if (status == SX_ST_GAVEUP) {
            fprintf(stderr, "error: gave up after %d timeouts with no progress\n", SX_GIVEUP_TIMEOUTS);
            goto done;
        }

        long rem = sx_timer_remaining_ms(&st, now);
        int pt = (int)(rem > POLL_CAP_MS ? POLL_CAP_MS : (rem < 1 ? 1 : rem));
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        if (poll(&pfd, 1, pt) < 0) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "error: poll failed: %s\n", strerror(errno));
            goto done;
        }
        for (;;) {
            ssize_t n = recvfrom(fd, rbuf, sizeof(rbuf), 0, NULL, NULL);
            if (n < 0) {
                if (errno == EINTR || errno == EAGAIN)
                    break;
                fprintf(stderr, "error: recvfrom failed: %s\n", strerror(errno));
                goto done;
            }
            if (n == 0)
                break;
            sx_on_packet(&st, now_ms(), rbuf, (size_t)n);
        }
        sx_tick(&st, now_ms());
    }

done:
    close(fd);
    free(file);
    return rc;
}

static int run_recv(const config_t *cfg)
{
    struct sockaddr_storage addr;
    socklen_t alen = 0;
    int fd = open_relay(cfg->relay, cfg->port, &addr, &alen);
    if (fd < 0)
        return 2;
    if (handshake(fd, &addr, alen, cfg->session, 0, 0.0, 0.0, 0.0) != 0) {
        close(fd);
        return 2;
    }

    FILE *f = fopen(cfg->file, "wb");
    if (f == NULL) {
        fprintf(stderr, "error: cannot open '%s' for writing: %s\n", cfg->file, strerror(errno));
        close(fd);
        return 2;
    }

    rx_state_t st;
    rx_init(&st, now_ms());

    uint8_t rbuf[IO_BUF_SIZE];
    uint8_t ackbuf[PKT_HEADER_LEN];
    int rc = 2;
    for (;;) {
        long now = now_ms();
        rx_action_t act = rx_tick(&st, now);
        if (act == RX_ACT_DONE) {
            rc = 0;
            break;
        }
        if (act == RX_ACT_GIVEUP) {
            fprintf(stderr, "error: no valid packet for %d ms\n", RX_IDLE_GIVEUP_MS);
            goto done;
        }

        long wait_ms = st.finished ? st.linger_until_ms - now : st.last_valid_ms + RX_IDLE_GIVEUP_MS - now;
        int pt = (int)(wait_ms > POLL_CAP_MS ? POLL_CAP_MS : (wait_ms < 1 ? 1 : wait_ms));
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        if (poll(&pfd, 1, pt) < 0) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "error: poll failed: %s\n", strerror(errno));
            goto done;
        }
        for (;;) {
            ssize_t n = recvfrom(fd, rbuf, sizeof(rbuf), 0, NULL, NULL);
            if (n < 0) {
                if (errno == EINTR || errno == EAGAIN)
                    break;
                fprintf(stderr, "error: recvfrom failed: %s\n", strerror(errno));
                goto done;
            }
            if (n == 0)
                break;
            rx_action_t a = rx_on_packet(&st, now_ms(), rbuf, (size_t)n);
            if (a == RX_ACT_WRITE_ACK && fwrite(st.payload, 1, st.payload_len, f) != st.payload_len) {
                fprintf(stderr, "error: short write to '%s'\n", cfg->file);
                goto done;
            }
            if (a == RX_ACT_WRITE_ACK || a == RX_ACT_ACK) {
                int alen2 = pkt_encode(ackbuf, sizeof(ackbuf), PKT_TYPE_ACK, st.expected, NULL, 0);
                if (sendto(fd, ackbuf, (size_t)alen2, 0, (const struct sockaddr *)&addr, alen) < 0) {
                    fprintf(stderr, "error: sendto failed: %s\n", strerror(errno));
                    goto done;
                }
            }
        }
    }

done:
    fclose(f);
    close(fd);
    return rc;
}

int main(int argc, char **argv)
{
    config_t cfg;
    int code = parse_args(argc, argv, &cfg);
    if (code != 0)
        return code;
    if (argc == 1)
        return 0;

    if (cfg.mode == MODE_SEND)
        return run_send(&cfg);
    return run_recv(&cfg);
}
