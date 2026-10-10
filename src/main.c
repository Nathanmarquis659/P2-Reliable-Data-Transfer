#include "lab.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef TEST
#define main main_exclude
#endif



int main(int argc, char **argv)
{
    config_t cfg;
    int code = parse_args(argc, argv, &cfg);
    if (code != 0)
        return code;
    if (argc == 1)
        return 0;

    printf("mode=%s\n", cfg.mode == MODE_SEND ? "send" : "recv");
    printf("session=%s\n", cfg.session);
    printf("window=%d\n", cfg.window);
    printf("timeout_ms=%d\n", cfg.timeout_ms);
    printf("loss=%.6f\n", cfg.loss);
    printf("corrupt=%.6f\n", cfg.corrupt);
    printf("dup=%.6f\n", cfg.dup);
    printf("port=%d\n", cfg.port);
    printf("relay=%s\n", cfg.relay);
    printf("file=%s\n", cfg.file);

    return 0;
}