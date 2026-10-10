#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "harness/unity.h"
#include "../src/lab.h"
#include "../src/pkt.h"
#include "../src/rx.h"
#include "../src/sx.h"

static void assert_double_near(double expected, double actual)
{
    double d = expected - actual;
    if (d < 0.0)
        d = -d;
    TEST_ASSERT_TRUE(d <= 1e-9);
}

void setUp(void) {}

void tearDown(void) {}

void test_no_args_returns_zero(void)
{
    config_t c;
    char *a[] = {"myapp"};
    TEST_ASSERT_EQUAL_INT(0, parse_args(1, a, &c));
}

void test_valid_send_all_flags(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc123", "-w", "16", "-T", "500",
                 "-l", "0.1", "-c", "0.2", "-d", "0.3", "-p", "5000",
                 "relay.example", "in.bin"};
    TEST_ASSERT_EQUAL_INT(0, parse_args(18, a, &c));
    TEST_ASSERT_EQUAL_INT(MODE_SEND, c.mode);
    TEST_ASSERT_EQUAL_STRING("abc123", c.session);
    TEST_ASSERT_EQUAL_INT(16, c.window);
    TEST_ASSERT_EQUAL_INT(500, c.timeout_ms);
    assert_double_near(0.1, c.loss);
    assert_double_near(0.2, c.corrupt);
    assert_double_near(0.3, c.dup);
    TEST_ASSERT_EQUAL_INT(5000, c.port);
    TEST_ASSERT_EQUAL_STRING("relay.example", c.relay);
    TEST_ASSERT_EQUAL_STRING("in.bin", c.file);
}

void test_valid_recv_defaults(void)
{
    config_t c;
    char *a[] = {"myapp", "recv", "-s", "abc", "relay", "out.bin"};
    TEST_ASSERT_EQUAL_INT(0, parse_args(6, a, &c));
    TEST_ASSERT_EQUAL_INT(MODE_RECV, c.mode);
    TEST_ASSERT_EQUAL_STRING("abc", c.session);
    TEST_ASSERT_EQUAL_INT(8, c.window);
    TEST_ASSERT_EQUAL_INT(250, c.timeout_ms);
    assert_double_near(0.0, c.loss);
    assert_double_near(0.0, c.corrupt);
    assert_double_near(0.0, c.dup);
    TEST_ASSERT_EQUAL_INT(4250, c.port);
    TEST_ASSERT_EQUAL_STRING("relay", c.relay);
    TEST_ASSERT_EQUAL_STRING("out.bin", c.file);
}

void test_bad_mode_returns_one(void)
{
    config_t c;
    char *a[] = {"myapp", "bogus", "-s", "abc", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(6, a, &c));
}

void test_window_zero_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc", "-w", "0", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(8, a, &c));
}

void test_window_too_large_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc", "-w", "65", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(8, a, &c));
}

void test_window_nonnumeric_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc", "-w", "abc", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(8, a, &c));
}

void test_rate_too_high_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc", "-l", "0.6", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(8, a, &c));
}

void test_rate_negative_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc", "-d", "-0.1", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(8, a, &c));
}

void test_rate_boundary_valid(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc", "-l", "0.5", "-c", "0", "-d", "0.5", "r", "f"};
    TEST_ASSERT_EQUAL_INT(0, parse_args(12, a, &c));
    assert_double_near(0.5, c.loss);
    assert_double_near(0.0, c.corrupt);
    assert_double_near(0.5, c.dup);
}

void test_port_out_of_range_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc", "-p", "65536", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(8, a, &c));
}

void test_missing_file_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc", "relay"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(5, a, &c));
}

void test_extra_positional_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "abc", "r", "f", "x"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(7, a, &c));
}

void test_send_flag_on_recv_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "recv", "-s", "abc", "-w", "4", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(8, a, &c));
}

void test_loss_flag_on_recv_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "recv", "-s", "abc", "-l", "0.1", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(8, a, &c));
}

void test_session_too_long_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "aaaaaaaaaabbbbbbbbbbccccccccccddd", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(6, a, &c));
}

void test_session_bad_charset_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "-s", "Abc", "r", "f"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(6, a, &c));
}

void test_missing_session_rejected(void)
{
    config_t c;
    char *a[] = {"myapp", "send", "relay", "file"};
    TEST_ASSERT_EQUAL_INT(1, parse_args(4, a, &c));
}

static uint8_t g_pkt[PKT_MAX_SIZE];

static int make_data(uint32_t seq, const uint8_t *payload, uint16_t len)
{
    return pkt_encode(g_pkt, sizeof(g_pkt), PKT_TYPE_DATA, seq, payload, len);
}

static int make_raw(int type, uint8_t reserved, uint32_t seq)
{
    uint8_t b[PKT_HEADER_LEN];
    memset(b, 0, sizeof(b));
    b[0] = (uint8_t)type;
    b[1] = reserved;
    b[4] = (uint8_t)(seq >> 24);
    b[5] = (uint8_t)(seq >> 16);
    b[6] = (uint8_t)(seq >> 8);
    b[7] = (uint8_t)seq;
    uint16_t c = pkt_checksum(b, sizeof(b));
    b[2] = (uint8_t)(c >> 8);
    b[3] = (uint8_t)c;
    memcpy(g_pkt, b, sizeof(b));
    return PKT_HEADER_LEN;
}

static int make_ack(uint32_t seq)
{
    return pkt_encode(g_pkt, sizeof(g_pkt), PKT_TYPE_ACK, seq, NULL, 0);
}

static int make_fin(uint32_t seq)
{
    return pkt_encode(g_pkt, sizeof(g_pkt), PKT_TYPE_FIN, seq, NULL, 0);
}

void test_checksum_rfc1071_example(void)
{
    uint8_t d[] = {0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7};
    TEST_ASSERT_EQUAL_UINT16(0x220d, pkt_checksum(d, sizeof(d)));
}

void test_checksum_odd_length(void)
{
    uint8_t d[] = {0x00, 0x01, 0xf2};
    TEST_ASSERT_EQUAL_UINT16(0x0dfe, pkt_checksum(d, sizeof(d)));
}

void test_encode_hi_seq2_exact_wire_bytes(void)
{
    uint8_t payload[] = {'H', 'i', '!'};
    int n = make_data(2, payload, 3);
    TEST_ASSERT_EQUAL_INT(13, n);
    uint8_t want[] = {0x00, 0x00, 0x96, 0x91, 0x00, 0x00, 0x00, 0x02,
                      0x00, 0x03, 0x48, 0x69, 0x21};
    TEST_ASSERT_EQUAL_MEMORY(want, g_pkt, sizeof(want));
}

void test_encode_rejects_bad_args(void)
{
    uint8_t b[PKT_MAX_SIZE];
    uint8_t payload[] = {0};
    TEST_ASSERT_EQUAL_INT(-1, pkt_encode(b, sizeof(b), 5, 0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, pkt_encode(b, sizeof(b), PKT_TYPE_DATA, 0, payload, (uint16_t)(PKT_MAX_PAYLOAD + 1)));
    TEST_ASSERT_EQUAL_INT(-1, pkt_encode(b, 5, PKT_TYPE_ACK, 0, NULL, 0));
}

void test_validate_roundtrip_max_payload(void)
{
    static uint8_t big[PKT_MAX_PAYLOAD];
    for (int i = 0; i < PKT_MAX_PAYLOAD; i++)
        big[i] = (uint8_t)(i * 7);
    int n = make_data(42, big, PKT_MAX_PAYLOAD);
    TEST_ASSERT_EQUAL_INT(PKT_HEADER_LEN + PKT_MAX_PAYLOAD, n);

    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(g_pkt, (size_t)n, &f));
    TEST_ASSERT_EQUAL_INT(PKT_TYPE_DATA, f.type);
    TEST_ASSERT_EQUAL_UINT32(42, f.seq);
    TEST_ASSERT_EQUAL_UINT16(PKT_MAX_PAYLOAD, f.length);
    TEST_ASSERT_EQUAL_UINT16(0x0000, pkt_checksum(g_pkt, (size_t)n));
}

void test_validate_rejects_too_short(void)
{
    uint8_t d[9] = {0};
    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(-1, pkt_validate(d, sizeof(d), &f));
    TEST_ASSERT_EQUAL_INT(-1, pkt_validate(d, 0, &f));
}

void test_validate_rejects_length_mismatch(void)
{
    uint8_t payload[] = {'H', 'i', '!'};
    int n = make_data(2, payload, 3);
    pkt_fields_t f;
    g_pkt[9] = 4;
    TEST_ASSERT_EQUAL_INT(-1, pkt_validate(g_pkt, (size_t)n, &f));
    make_data(2, payload, 3);
    g_pkt[9] = 2;
    TEST_ASSERT_EQUAL_INT(-1, pkt_validate(g_pkt, (size_t)n, &f));
}

void test_validate_rejects_length_over_max(void)
{
    static uint8_t big[PKT_HEADER_LEN + 1025];
    memset(big, 0x5a, sizeof(big));
    big[0] = PKT_TYPE_DATA;
    big[8] = (uint8_t)(1025 >> 8);
    big[9] = (uint8_t)1025;
    uint16_t c = pkt_checksum(big, sizeof(big));
    big[2] = (uint8_t)(c >> 8);
    big[3] = (uint8_t)c;
    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(-1, pkt_validate(big, sizeof(big), &f));
}

void test_validate_rejects_unknown_type(void)
{
    int n = make_raw(5, 0, 0);
    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(-1, pkt_validate(g_pkt, (size_t)n, &f));
}

void test_validate_rejects_reserved(void)
{
    int n = make_raw(PKT_TYPE_ACK, 1, 3);
    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(-1, pkt_validate(g_pkt, (size_t)n, &f));
}

void test_validate_rejects_bit_flip_payload(void)
{
    uint8_t payload[] = {'H', 'i', '!'};
    int n = make_data(2, payload, 3);
    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(g_pkt, (size_t)n, &f));
    g_pkt[PKT_HEADER_LEN + 1] ^= 0x01;
    TEST_ASSERT_EQUAL_INT(-1, pkt_validate(g_pkt, (size_t)n, &f));
}

void test_validate_rejects_bit_flip_header(void)
{
    uint8_t payload[] = {'H', 'i', '!'};
    int n = make_data(2, payload, 3);
    pkt_fields_t f;
    g_pkt[6] ^= 0x40;
    TEST_ASSERT_EQUAL_INT(-1, pkt_validate(g_pkt, (size_t)n, &f));
}

void test_rx_init_state(void)
{
    rx_state_t st;
    rx_init(&st, 123);
    TEST_ASSERT_EQUAL_UINT32(0, st.expected);
    TEST_ASSERT_FALSE(st.finished);
    TEST_ASSERT_NULL(st.payload);
    TEST_ASSERT_EQUAL_INT64(123, (int64_t)st.last_valid_ms);
}

void test_rx_in_order_data_then_fin_worked_example(void)
{
    rx_state_t st;
    rx_init(&st, 0);

    uint8_t p1[] = {'a', 'b'};
    TEST_ASSERT_EQUAL_INT(12, make_data(0, p1, 2));
    TEST_ASSERT_EQUAL(RX_ACT_WRITE_ACK, rx_on_packet(&st, 0, g_pkt, 12));
    TEST_ASSERT_EQUAL_UINT32(1, st.expected);
    TEST_ASSERT_EQUAL_MEMORY(p1, st.payload, 2);

    uint8_t p2[] = {'c'};
    TEST_ASSERT_EQUAL_INT(11, make_data(1, p2, 1));
    TEST_ASSERT_EQUAL(RX_ACT_WRITE_ACK, rx_on_packet(&st, 0, g_pkt, 11));
    TEST_ASSERT_EQUAL_UINT32(2, st.expected);

    TEST_ASSERT_EQUAL_INT(10, make_fin(2));
    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 5, g_pkt, 10));
    TEST_ASSERT_TRUE(st.finished);
    TEST_ASSERT_EQUAL_UINT32(3, st.expected);
    TEST_ASSERT_EQUAL_INT64(2005, (int64_t)st.linger_until_ms);

    TEST_ASSERT_EQUAL(RX_ACT_NONE, rx_tick(&st, 1999));
    TEST_ASSERT_EQUAL(RX_ACT_DONE, rx_tick(&st, 2005));
}

void test_rx_duplicate_and_out_of_order_reack(void)
{
    rx_state_t st;
    rx_init(&st, 0);

    uint8_t p[] = {'x', 'y'};
    make_data(1, p, 2);
    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 0, g_pkt, 12));
    TEST_ASSERT_EQUAL_UINT32(0, st.expected);

    make_data(0, p, 2);
    TEST_ASSERT_EQUAL(RX_ACT_WRITE_ACK, rx_on_packet(&st, 0, g_pkt, 12));
    TEST_ASSERT_EQUAL_UINT32(1, st.expected);

    make_data(0, p, 2);
    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 0, g_pkt, 12));
    TEST_ASSERT_EQUAL_UINT32(1, st.expected);
}

void test_rx_invalid_datagram_ignored(void)
{
    rx_state_t st;
    rx_init(&st, 0);
    uint8_t d[5] = {1, 2, 3, 4, 5};
    TEST_ASSERT_EQUAL(RX_ACT_NONE, rx_on_packet(&st, 100, d, sizeof(d)));
    TEST_ASSERT_EQUAL_INT(0, st.last_valid_ms);
}

void test_rx_ack_at_receiver_ignored(void)
{
    rx_state_t st;
    rx_init(&st, 0);
    TEST_ASSERT_EQUAL_INT(10, make_ack(0));
    TEST_ASSERT_EQUAL(RX_ACT_NONE, rx_on_packet(&st, 7, g_pkt, 10));
    TEST_ASSERT_EQUAL_UINT32(0, st.expected);
}

void test_rx_fin_wrong_seq_reacked(void)
{
    rx_state_t st;
    rx_init(&st, 0);
    uint8_t p[] = {'a'};
    make_data(0, p, 1);
    TEST_ASSERT_EQUAL(RX_ACT_WRITE_ACK, rx_on_packet(&st, 0, g_pkt, 11));

    TEST_ASSERT_EQUAL_INT(10, make_fin(5));
    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 0, g_pkt, 10));
    TEST_ASSERT_FALSE(st.finished);
    TEST_ASSERT_EQUAL_UINT32(1, st.expected);
}

void test_rx_after_finished_valid_packets_ack_only(void)
{
    rx_state_t st;
    rx_init(&st, 0);
    TEST_ASSERT_EQUAL_INT(10, make_fin(0));
    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 0, g_pkt, 10));

    uint8_t p[] = {'z'};
    make_data(99, p, 1);
    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 10, g_pkt, 11));
    TEST_ASSERT_TRUE(st.finished);
}

void test_rx_idle_giveup_and_reset(void)
{
    rx_state_t st;
    rx_init(&st, 0);
    TEST_ASSERT_EQUAL(RX_ACT_NONE, rx_tick(&st, 29999));
    TEST_ASSERT_EQUAL(RX_ACT_GIVEUP, rx_tick(&st, 30000));

    uint8_t p[] = {'q'};
    make_data(0, p, 1);
    TEST_ASSERT_EQUAL(RX_ACT_WRITE_ACK, rx_on_packet(&st, 30000, g_pkt, 11));
    TEST_ASSERT_EQUAL(RX_ACT_NONE, rx_tick(&st, 59999));
}

void test_rx_linger_ignores_data(void)
{
    rx_state_t st;
    rx_init(&st, 0);
    TEST_ASSERT_EQUAL_INT(10, make_fin(0));
    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 0, g_pkt, 10));

    uint8_t p[] = {'w'};
    make_data(0, p, 1);
    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 100, g_pkt, 11));
    TEST_ASSERT_TRUE(st.finished);
    TEST_ASSERT_EQUAL_INT64(2000, (int64_t)st.linger_until_ms);

    TEST_ASSERT_EQUAL(RX_ACT_DONE, rx_tick(&st, 2000));
}

void test_rx_duplicate_fin_acks_linger_unchanged(void)
{
    rx_state_t st;
    rx_init(&st, 0);
    TEST_ASSERT_EQUAL_INT(10, make_fin(0));
    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 0, g_pkt, 10));
    TEST_ASSERT_EQUAL_INT64(2000, (int64_t)st.linger_until_ms);

    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 500, g_pkt, make_fin(0)));
    TEST_ASSERT_EQUAL_INT64(2000, (int64_t)st.linger_until_ms);
}

void test_rx_worked_example_2500_bytes(void)
{
    rx_state_t st;
    rx_init(&st, 0);

    static uint8_t full[1024];
    static uint8_t tail[452];
    memset(full, 0x42, sizeof(full));
    memset(tail, 0x42, sizeof(tail));

    TEST_ASSERT_EQUAL(RX_ACT_WRITE_ACK, rx_on_packet(&st, 0, g_pkt, make_data(0, full, 1024)));
    TEST_ASSERT_EQUAL(RX_ACT_WRITE_ACK, rx_on_packet(&st, 0, g_pkt, make_data(1, full, 1024)));
    TEST_ASSERT_EQUAL(RX_ACT_WRITE_ACK, rx_on_packet(&st, 0, g_pkt, make_data(2, tail, 452)));

    TEST_ASSERT_EQUAL(RX_ACT_ACK, rx_on_packet(&st, 0, g_pkt, make_fin(3)));
    TEST_ASSERT_TRUE(st.finished);
    TEST_ASSERT_EQUAL_UINT32(4, st.expected);
}

static uint8_t g_file_10k[10 * PKT_MAX_PAYLOAD];
static uint8_t g_file_3k[3 * PKT_MAX_PAYLOAD];
static uint8_t g_file_2k[2 * PKT_MAX_PAYLOAD];

static void fill_pattern(uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
        buf[i] = (uint8_t)(i % 251u);
}

void test_sx_init_n_data(void)
{
    static sx_state_t st;
    uint8_t one = 0;
    uint8_t k1[PKT_MAX_PAYLOAD];
    uint8_t k1p[PKT_MAX_PAYLOAD + 1];

    sx_init(&st, 8, 250, NULL, 0);
    TEST_ASSERT_EQUAL_UINT32(0, st.n_data);
    sx_init(&st, 8, 250, &one, 1);
    TEST_ASSERT_EQUAL_UINT32(1, st.n_data);
    sx_init(&st, 8, 250, k1, sizeof(k1));
    TEST_ASSERT_EQUAL_UINT32(1, st.n_data);
    sx_init(&st, 8, 250, k1p, sizeof(k1p));
    TEST_ASSERT_EQUAL_UINT32(2, st.n_data);

    fill_pattern(g_file_2k, sizeof(g_file_2k));
    sx_init(&st, 8, 250, g_file_2k, sizeof(g_file_2k));
    TEST_ASSERT_EQUAL_UINT32(2, st.n_data);

    fill_pattern(g_file_10k, sizeof(g_file_10k));
    sx_init(&st, 4, 250, g_file_10k, sizeof(g_file_10k));
    TEST_ASSERT_EQUAL_UINT32(10, st.n_data);
    TEST_ASSERT_EQUAL_UINT32(0, st.base);
    TEST_ASSERT_EQUAL_UINT32(0, st.next);
    TEST_ASSERT_FALSE(st.timer_running);
    TEST_ASSERT_EQUAL_INT(SX_ST_ACTIVE, sx_status(&st));
    TEST_ASSERT_EQUAL_INT64(0, (int64_t)sx_timer_remaining_ms(&st, 0));
}

void test_sx_initial_burst_fills_window(void)
{
    static sx_state_t st;
    fill_pattern(g_file_10k, sizeof(g_file_10k));
    sx_init(&st, 4, 250, g_file_10k, sizeof(g_file_10k));

    sx_send_t s;
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));
        pkt_fields_t f;
        TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f));
        TEST_ASSERT_EQUAL_INT(PKT_TYPE_DATA, f.type);
        TEST_ASSERT_EQUAL_UINT32((uint32_t)i, f.seq);
        TEST_ASSERT_EQUAL_UINT16(PKT_MAX_PAYLOAD, f.length);
    }
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 0, &s));
    TEST_ASSERT_TRUE(st.timer_running);
    TEST_ASSERT_EQUAL_INT64(250, (int64_t)st.deadline_ms);
    TEST_ASSERT_EQUAL_INT64(250, (int64_t)sx_timer_remaining_ms(&st, 0));
    TEST_ASSERT_EQUAL_INT64(100, (int64_t)sx_timer_remaining_ms(&st, 150));
}

void test_sx_ack_slides_window_and_refills(void)
{
    static sx_state_t st;
    fill_pattern(g_file_10k, sizeof(g_file_10k));
    sx_init(&st, 4, 250, g_file_10k, sizeof(g_file_10k));

    sx_send_t s;
    for (int i = 0; i < 4; i++)
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));

    int n = make_ack(4);
    sx_on_packet(&st, 10, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(4, st.base);
    TEST_ASSERT_FALSE(st.timer_running);

    for (uint32_t seq = 4; seq < 8; seq++) {
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 10, &s));
        pkt_fields_t f;
        TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f));
        TEST_ASSERT_EQUAL_UINT32(seq, f.seq);
    }
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 10, &s));
    TEST_ASSERT_TRUE(st.timer_running);
    TEST_ASSERT_EQUAL_INT64(260, (int64_t)st.deadline_ms);

    n = make_ack(8);
    sx_on_packet(&st, 20, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(8, st.base);

    for (uint32_t seq = 8; seq < 10; seq++) {
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 20, &s));
        pkt_fields_t f;
        TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f));
        TEST_ASSERT_EQUAL_UINT32(seq, f.seq);
    }
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 20, &s));

    n = make_ack(10);
    sx_on_packet(&st, 30, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(10, st.base);
    TEST_ASSERT_FALSE(st.timer_running);

    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 30, &s));
    pkt_fields_t ff;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &ff));
    TEST_ASSERT_EQUAL_INT(PKT_TYPE_FIN, ff.type);
    TEST_ASSERT_EQUAL_UINT32(10, ff.seq);
    TEST_ASSERT_EQUAL_UINT16(0, ff.length);
}

void test_sx_duplicate_ack_ignored(void)
{
    static sx_state_t st;
    fill_pattern(g_file_10k, sizeof(g_file_10k));
    sx_init(&st, 8, 250, g_file_10k, sizeof(g_file_10k));

    sx_send_t s;
    for (int i = 0; i < 8; i++)
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));

    int n = make_ack(4);
    sx_on_packet(&st, 10, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(4, st.base);
    TEST_ASSERT_TRUE(st.timer_running);
    TEST_ASSERT_EQUAL_INT64(260, (int64_t)st.deadline_ms);

    n = make_ack(4);
    sx_on_packet(&st, 50, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(4, st.base);
    TEST_ASSERT_EQUAL_INT64(260, (int64_t)st.deadline_ms);

    n = make_ack(2);
    sx_on_packet(&st, 60, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(4, st.base);
    TEST_ASSERT_EQUAL_INT64(260, (int64_t)st.deadline_ms);
}

void test_sx_timeout_resends_full_window(void)
{
    static sx_state_t st;
    fill_pattern(g_file_10k, sizeof(g_file_10k));
    sx_init(&st, 4, 250, g_file_10k, sizeof(g_file_10k));

    sx_send_t s;
    for (int i = 0; i < 4; i++)
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 0, &s));

    sx_tick(&st, 249);
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 249, &s));

    sx_tick(&st, 250);
    TEST_ASSERT_EQUAL_INT64(500, (int64_t)st.deadline_ms);
    TEST_ASSERT_EQUAL_INT(1, st.stuck_timeouts);

    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 250, &s));
        pkt_fields_t f;
        TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f));
        TEST_ASSERT_EQUAL_UINT32((uint32_t)i, f.seq);
        TEST_ASSERT_EQUAL_MEMORY(g_file_10k + (size_t)i * PKT_MAX_PAYLOAD, s.buf + PKT_HEADER_LEN, PKT_MAX_PAYLOAD);
    }
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 250, &s));
}

void test_sx_giveup_after_10_timeouts(void)
{
    static sx_state_t st;
    fill_pattern(g_file_10k, sizeof(g_file_10k));
    sx_init(&st, 4, 250, g_file_10k, sizeof(g_file_10k));

    sx_send_t s;
    for (int i = 0; i < 4; i++)
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));

    for (int i = 1; i <= SX_GIVEUP_TIMEOUTS; i++) {
        sx_tick(&st, 250L * i);
        if (i < SX_GIVEUP_TIMEOUTS)
            TEST_ASSERT_EQUAL_INT(SX_ST_ACTIVE, sx_status(&st));
    }
    TEST_ASSERT_EQUAL_INT(SX_ST_GAVEUP, sx_status(&st));

    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 3000, &s));
    int n = make_ack(4);
    sx_on_packet(&st, 3100, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(0, st.base);
    sx_tick(&st, 5000);
    TEST_ASSERT_EQUAL_INT(SX_ST_GAVEUP, sx_status(&st));
}

void test_sx_giveup_streak_resets_on_progress(void)
{
    static sx_state_t st;
    fill_pattern(g_file_10k, sizeof(g_file_10k));
    sx_init(&st, 4, 250, g_file_10k, sizeof(g_file_10k));

    sx_send_t s;
    for (int i = 0; i < 4; i++)
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));

    sx_tick(&st, 250);
    sx_tick(&st, 500);
    sx_tick(&st, 750);
    TEST_ASSERT_EQUAL_INT(3, st.stuck_timeouts);
    TEST_ASSERT_EQUAL_INT(SX_ST_ACTIVE, sx_status(&st));
    for (int i = 0; i < 4; i++)
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 750, &s));
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 750, &s));

    int n = make_ack(4);
    sx_on_packet(&st, 800, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(4, st.base);
    TEST_ASSERT_FALSE(st.timer_running);

    for (uint32_t seq = 4; seq < 8; seq++)
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 800, &s));
    TEST_ASSERT_TRUE(st.timer_running);
    TEST_ASSERT_EQUAL_INT64(1050, (int64_t)st.deadline_ms);

    for (int i = 0; i < SX_GIVEUP_TIMEOUTS - 1; i++) {
        sx_tick(&st, 1050L + 250L * i);
        TEST_ASSERT_EQUAL_INT(SX_ST_ACTIVE, sx_status(&st));
    }
    sx_tick(&st, 1050L + 250L * (SX_GIVEUP_TIMEOUTS - 1));
    TEST_ASSERT_EQUAL_INT(SX_ST_GAVEUP, sx_status(&st));
}

void test_sx_fin_flow_single_chunk(void)
{
    static sx_state_t st;
    uint8_t f[5] = {1, 2, 3, 4, 5};
    sx_init(&st, 8, 250, f, sizeof(f));
    TEST_ASSERT_EQUAL_UINT32(1, st.n_data);

    sx_send_t s;
    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));
    pkt_fields_t d;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &d));
    TEST_ASSERT_EQUAL_INT(PKT_TYPE_DATA, d.type);
    TEST_ASSERT_EQUAL_UINT32(0, d.seq);
    TEST_ASSERT_EQUAL_UINT16(5, d.length);
    TEST_ASSERT_EQUAL_MEMORY(f, s.buf + PKT_HEADER_LEN, 5);
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 0, &s));

    int n = make_ack(1);
    sx_on_packet(&st, 5, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(1, st.base);
    TEST_ASSERT_FALSE(st.timer_running);
    TEST_ASSERT_EQUAL_INT64(0, (int64_t)sx_timer_remaining_ms(&st, 5));

    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 5, &s));
    pkt_fields_t fp;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &fp));
    TEST_ASSERT_EQUAL_INT(PKT_TYPE_FIN, fp.type);
    TEST_ASSERT_EQUAL_UINT32(1, fp.seq);
    TEST_ASSERT_TRUE(st.timer_running);
    TEST_ASSERT_EQUAL_INT64(255, (int64_t)st.deadline_ms);

    n = make_ack(2);
    sx_on_packet(&st, 30, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_INT(SX_ST_DONE, sx_status(&st));
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 30, &s));

    n = make_ack(2);
    sx_on_packet(&st, 40, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_INT(SX_ST_DONE, sx_status(&st));
}

void test_sx_empty_file_fin_seq_zero(void)
{
    static sx_state_t st;
    uint8_t dummy = 0;
    sx_init(&st, 8, 250, &dummy, 0);

    sx_send_t s;
    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));
    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f));
    TEST_ASSERT_EQUAL_INT(PKT_TYPE_FIN, f.type);
    TEST_ASSERT_EQUAL_UINT32(0, f.seq);
    TEST_ASSERT_EQUAL_UINT16(0, f.length);

    int n = make_ack(1);
    sx_on_packet(&st, 7, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_INT(SX_ST_DONE, sx_status(&st));
}

void test_sx_fin_retransmit_on_timeout(void)
{
    static sx_state_t st;
    uint8_t f[5] = {9, 8, 7, 6, 5};
    sx_init(&st, 8, 250, f, sizeof(f));

    sx_send_t s;
    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));
    int n = make_ack(1);
    sx_on_packet(&st, 5, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 5, &s));
    TEST_ASSERT_TRUE(st.timer_running);

    sx_tick(&st, 260);
    TEST_ASSERT_EQUAL_INT(SX_ST_ACTIVE, sx_status(&st));

    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 260, &s));
    pkt_fields_t fp;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &fp));
    TEST_ASSERT_EQUAL_INT(PKT_TYPE_FIN, fp.type);
    TEST_ASSERT_EQUAL_UINT32(1, fp.seq);
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 260, &s));

    sx_tick(&st, 510);
    TEST_ASSERT_EQUAL_INT(SX_ST_ACTIVE, sx_status(&st));
}

void test_sx_foreign_and_invalid_packets_ignored(void)
{
    static sx_state_t st;
    fill_pattern(g_file_10k, sizeof(g_file_10k));
    sx_init(&st, 4, 250, g_file_10k, sizeof(g_file_10k));

    uint8_t garbage[5] = {9, 8, 7, 6, 5};
    sx_on_packet(&st, 0, garbage, sizeof(garbage));
    uint8_t p[] = {'x', 'y'};
    make_data(7, p, 2);
    sx_on_packet(&st, 0, g_pkt, 12);
    make_fin(3);
    sx_on_packet(&st, 0, g_pkt, 10);

    TEST_ASSERT_EQUAL_UINT32(0, st.base);
    TEST_ASSERT_EQUAL_UINT32(0, st.next);
    TEST_ASSERT_FALSE(st.timer_running);
    TEST_ASSERT_EQUAL_INT(SX_ST_ACTIVE, sx_status(&st));

    sx_send_t s;
    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));
    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f));
    TEST_ASSERT_EQUAL_UINT32(0, f.seq);
}

void test_sx_ack_above_n_data_clamped(void)
{
    static sx_state_t st;
    fill_pattern(g_file_2k, sizeof(g_file_2k));
    sx_init(&st, 8, 250, g_file_2k, sizeof(g_file_2k));

    sx_send_t s;
    for (int i = 0; i < 2; i++)
        TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 0, &s));

    int n = make_ack(7);
    sx_on_packet(&st, 5, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_UINT32(2, st.base);
    TEST_ASSERT_FALSE(st.timer_running);

    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 5, &s));
    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f));
    TEST_ASSERT_EQUAL_INT(PKT_TYPE_FIN, f.type);
    TEST_ASSERT_EQUAL_UINT32(2, f.seq);

    n = make_ack(3);
    sx_on_packet(&st, 9, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_INT(SX_ST_DONE, sx_status(&st));
}

void test_sx_stop_and_wait_w1(void)
{
    static sx_state_t st;
    fill_pattern(g_file_3k, sizeof(g_file_3k));
    sx_init(&st, 1, 250, g_file_3k, sizeof(g_file_3k));

    sx_send_t s;
    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 0, &s));
    pkt_fields_t f;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f));
    TEST_ASSERT_EQUAL_UINT32(0, f.seq);
    TEST_ASSERT_EQUAL_INT(0, sx_next_send(&st, 0, &s));

    int n = make_ack(1);
    sx_on_packet(&st, 5, g_pkt, (size_t)n);
    TEST_ASSERT_FALSE(st.timer_running);
    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 5, &s));
    TEST_ASSERT_EQUAL_UINT32(2, st.next);

    n = make_ack(2);
    sx_on_packet(&st, 10, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 10, &s));
    pkt_fields_t f2;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f2));
    TEST_ASSERT_EQUAL_UINT32(2, f2.seq);

    n = make_ack(3);
    sx_on_packet(&st, 15, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_INT(1, sx_next_send(&st, 15, &s));
    pkt_fields_t f3;
    TEST_ASSERT_EQUAL_INT(0, pkt_validate(s.buf, (size_t)s.len, &f3));
    TEST_ASSERT_EQUAL_INT(PKT_TYPE_FIN, f3.type);
    TEST_ASSERT_EQUAL_UINT32(3, f3.seq);

    n = make_ack(4);
    sx_on_packet(&st, 20, g_pkt, (size_t)n);
    TEST_ASSERT_EQUAL_INT(SX_ST_DONE, sx_status(&st));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_no_args_returns_zero);
    RUN_TEST(test_valid_send_all_flags);
    RUN_TEST(test_valid_recv_defaults);
    RUN_TEST(test_bad_mode_returns_one);
    RUN_TEST(test_window_zero_rejected);
    RUN_TEST(test_window_too_large_rejected);
    RUN_TEST(test_window_nonnumeric_rejected);
    RUN_TEST(test_rate_too_high_rejected);
    RUN_TEST(test_rate_negative_rejected);
    RUN_TEST(test_rate_boundary_valid);
    RUN_TEST(test_port_out_of_range_rejected);
    RUN_TEST(test_missing_file_rejected);
    RUN_TEST(test_extra_positional_rejected);
    RUN_TEST(test_send_flag_on_recv_rejected);
    RUN_TEST(test_loss_flag_on_recv_rejected);
    RUN_TEST(test_session_too_long_rejected);
    RUN_TEST(test_session_bad_charset_rejected);
    RUN_TEST(test_missing_session_rejected);
    RUN_TEST(test_checksum_rfc1071_example);
    RUN_TEST(test_checksum_odd_length);
    RUN_TEST(test_encode_hi_seq2_exact_wire_bytes);
    RUN_TEST(test_encode_rejects_bad_args);
    RUN_TEST(test_validate_roundtrip_max_payload);
    RUN_TEST(test_validate_rejects_too_short);
    RUN_TEST(test_validate_rejects_length_mismatch);
    RUN_TEST(test_validate_rejects_length_over_max);
    RUN_TEST(test_validate_rejects_unknown_type);
    RUN_TEST(test_validate_rejects_reserved);
    RUN_TEST(test_validate_rejects_bit_flip_payload);
    RUN_TEST(test_validate_rejects_bit_flip_header);
    RUN_TEST(test_rx_init_state);
    RUN_TEST(test_rx_in_order_data_then_fin_worked_example);
    RUN_TEST(test_rx_duplicate_and_out_of_order_reack);
    RUN_TEST(test_rx_invalid_datagram_ignored);
    RUN_TEST(test_rx_ack_at_receiver_ignored);
    RUN_TEST(test_rx_fin_wrong_seq_reacked);
    RUN_TEST(test_rx_after_finished_valid_packets_ack_only);
    RUN_TEST(test_rx_idle_giveup_and_reset);
    RUN_TEST(test_rx_linger_ignores_data);
    RUN_TEST(test_rx_duplicate_fin_acks_linger_unchanged);
    RUN_TEST(test_rx_worked_example_2500_bytes);

    RUN_TEST(test_sx_init_n_data);
    RUN_TEST(test_sx_initial_burst_fills_window);
    RUN_TEST(test_sx_ack_slides_window_and_refills);
    RUN_TEST(test_sx_duplicate_ack_ignored);
    RUN_TEST(test_sx_timeout_resends_full_window);
    RUN_TEST(test_sx_giveup_after_10_timeouts);
    RUN_TEST(test_sx_giveup_streak_resets_on_progress);
    RUN_TEST(test_sx_fin_flow_single_chunk);
    RUN_TEST(test_sx_empty_file_fin_seq_zero);
    RUN_TEST(test_sx_fin_retransmit_on_timeout);
    RUN_TEST(test_sx_foreign_and_invalid_packets_ignored);
    RUN_TEST(test_sx_ack_above_n_data_clamped);
    RUN_TEST(test_sx_stop_and_wait_w1);

    return UNITY_END();
}