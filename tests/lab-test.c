#include <stdlib.h>
#include <stdio.h>
#include "harness/unity.h"
#include "../src/lab.h"

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
    return UNITY_END();
}
