#ifndef LAB_H
#define LAB_H

#include <netdb.h> /* NI_MAXHOST */

#define MAX_FILE_PATH 4096

typedef enum {
    MODE_SEND = 0,
    MODE_RECV = 1
} app_mode_t;

typedef struct {
    app_mode_t mode;
    char session[33];
    int window;
    int timeout_ms;
    double loss;
    double corrupt;
    double dup;
    int port;
    char relay[NI_MAXHOST];
    char file[MAX_FILE_PATH];
} config_t;

/** * @brief Returns a greeting message.
 *
 * This function returns a string that contains a greeting message.
 * The string is allocated with malloc and should be freed by the caller.
 * @param name The name to include in the greeting.
 * @return A greeting string.
 */
char* get_greeting(const char* restrict name);

/**
 * Parses the command line into *out following the spec in PLAN.md §3.
 * Returns the process exit code: 0 = valid (or no-args usage), 1 = bad command line.
 */
int parse_args(int argc, char **argv, config_t *out);

#endif // LAB_H
