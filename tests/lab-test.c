#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "harness/unity.h"
#include "../src/lab.h"
#include "../src/pkt.h"

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
    return UNITY_END();
}
