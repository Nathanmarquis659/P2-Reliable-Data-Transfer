#include "lab.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_usage(FILE *stream)
{
    fprintf(stream,
            "Usage:\n"
            "  myapp send -s <session> [-w window] [-T timeout-ms] [-l loss] [-c corrupt] [-d dup] [-p port] <relay> <file>\n"
            "  myapp recv -s <session> [-p port] <relay> <file>\n");
}

static int parse_int(const char *s, int lo, int hi, int *out)
{
    if (s == NULL)
        return 1;
    char *end = NULL;
    long val = strtol(s, &end, 10);
    if (end == s || *end != '\0')
        return 1;
    if (val < lo || val > hi)
        return 1;
    *out = (int) val;
    return 0;
}

static int parse_double(const char *s, double lo, double hi, double *out)
{
    if (s == NULL)
        return 1;
    char *end = NULL;
    double val = strtod(s, &end);
    if (end == s || *end != '\0')
        return 1;
    if (val < lo || val > hi)
        return 1;
    *out = val;
    return 0;
}

#ifdef TEST
int lab_test_parse_int(const char *s, int lo, int hi, int *out)
{
    return parse_int(s, lo, hi, out);
}

int lab_test_parse_double(const char *s, double lo, double hi, double *out)
{
    return parse_double(s, lo, hi, out);
}
#endif

static int valid_session(const char *s)
{
    size_t len = strlen(s);
    if (len < 1 || len > 32)
        return 0;
    for (size_t i = 0; i < len; i++)
    {
        char c = s[i];
        int ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
        if (!ok)
            return 0;
    }
    return 1;
}

int parse_args(int argc, char **argv, config_t *out)
{
    out->mode = MODE_SEND;
    out->window = 8;
    out->timeout_ms = 250;
    out->loss = 0.0;
    out->corrupt = 0.0;
    out->dup = 0.0;
    out->port = 4250;
    out->session[0] = '\0';
    out->relay[0] = '\0';
    out->file[0] = '\0';

    if (argc < 2)
    {
        print_usage(stdout);
        return 0;
    }

    const char *mode_str = argv[1];
    int is_send = 0;
    if (strcmp(mode_str, "send") == 0)
    {
        out->mode = MODE_SEND;
        is_send = 1;
    }
    else if (strcmp(mode_str, "recv") == 0)
    {
        out->mode = MODE_RECV;
    }
    else
    {
        fprintf(stderr, "error: unknown mode '%s'\n", mode_str);
        print_usage(stderr);
        return 1;
    }

    optind = 0;
    int c;
    while ((c = getopt(argc - 1, argv + 1, "s:w:T:l:c:d:p:")) != -1)
    {
        switch (c)
        {
            case 's':
                if (!valid_session(optarg))
                {
                    fprintf(stderr, "error: invalid session '%s'\n", optarg);
                    return 1;
                }
                snprintf(out->session, sizeof(out->session), "%s", optarg);
                break;
            case 'w':
                if (!is_send)
                {
                    fprintf(stderr, "error: -w is only valid for send\n");
                    return 1;
                }
                if (parse_int(optarg, 1, 64, &out->window))
                {
                    fprintf(stderr, "error: invalid -w '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'T':
                if (parse_int(optarg, 1, INT_MAX, &out->timeout_ms))
                {
                    fprintf(stderr, "error: invalid -T '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'l':
                if (!is_send)
                {
                    fprintf(stderr, "error: -l is only valid for send\n");
                    return 1;
                }
                if (parse_double(optarg, 0.0, 0.5, &out->loss))
                {
                    fprintf(stderr, "error: invalid -l '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'c':
                if (!is_send)
                {
                    fprintf(stderr, "error: -c is only valid for send\n");
                    return 1;
                }
                if (parse_double(optarg, 0.0, 0.5, &out->corrupt))
                {
                    fprintf(stderr, "error: invalid -c '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'd':
                if (!is_send)
                {
                    fprintf(stderr, "error: -d is only valid for send\n");
                    return 1;
                }
                if (parse_double(optarg, 0.0, 0.5, &out->dup))
                {
                    fprintf(stderr, "error: invalid -d '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'p':
                if (parse_int(optarg, 1, 65535, &out->port))
                {
                    fprintf(stderr, "error: invalid -p '%s'\n", optarg);
                    return 1;
                }
                break;
            default:
                print_usage(stderr);
                return 1;
        }
    }

    int npos = (argc - 1) - optind;
    if (npos != 2)
    {
        fprintf(stderr, "error: expected exactly two positional arguments <relay> <file>\n");
        print_usage(stderr);
        return 1;
    }
    snprintf(out->relay, sizeof(out->relay), "%s", (argv + 1)[optind]);
    snprintf(out->file, sizeof(out->file), "%s", (argv + 1)[optind + 1]);

    if (out->session[0] == '\0')
    {
        fprintf(stderr, "error: -s <session> is required\n");
        print_usage(stderr);
        return 1;
    }

    return 0;
}
