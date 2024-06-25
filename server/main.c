#include <argp.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "tc2.h"
#include "version.h"

const char *argp_program_version = "tinyc2" SERVER_VERSION;
static char doc[] = "A tiny C2 Framework";
static char args_doc[] = "";
static struct argp_option options[] = { 
    { "server", 's', "SERVER", 0, "IP of the server. Default: 0.0.0.0"},
    { "port", 'p', "PORT", 0, "Port of the server. Default: 8083"},
    { 0 } 
};

struct arguments {
    char* ip;
    uint16_t port;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
        case 's': arguments->ip = arg; break;
        case 'p': {
            errno = 0;
            unsigned long val = strtoul(arg, 0, 10);

            switch (errno) {
                case ERANGE:
                    printf("Invalid port. The data could not be represented as an unsigned long.\n");
                    return 1;
                // host-specific (GNU/Linux in my case)
                case EINVAL:
                    printf("Invalid port. Unsupported base / radix.\n");
                    return 1;
            }

            if (val > USHRT_MAX ) {
                printf("Invalid port. Value out of range.\n");
                return 1;
            }

            arguments->port = (short) val;
            break;
        }
        case ARGP_KEY_ARG: return 0;
        default: return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

int main(int argc, char* argv[]) {

    struct arguments arguments;
    arguments.ip = "0.0.0.0";
    arguments.port = 8083;

    int res = argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (res != 0) {
        return 1;
    }

    printf("Parsed successfully\n");

    int res2 = test();

    printf("%d\n", res2);

    return 0;
}