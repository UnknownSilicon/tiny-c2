#define _GNU_SOURCE
#include <argp.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "cli.h"
#include "ipc.h"
#include "server.h"
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


void handle_sigsegv(int sig) {
    printf("SIGSEGV PID: %d\n", getpid());
    signal(SIGSEGV, SIG_DFL);
    raise(SIGSEGV);
}

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

    struct hostent* hostname = gethostbyname(arguments.ip);

    printf("Starting server on host %s %d\n", arguments.ip, arguments.port);

    signal(SIGCHLD, handle_sigchld);
    signal(SIGSEGV, handle_sigsegv);

    // Create a shared, anonymous RW memory map for the init structure.
    struct message_queues *i_map = (struct message_queues*) mmap(NULL, sizeof(struct message_queues), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (i_map == MAP_FAILED) {
        printf("Mmap failed %d\n", errno);
        return 1;
    }

    for (int i=0; i<NUM_UP_QUEUES; i++) {
        // Initialize semaphore with 1, since nothing has to read or write immediately 
        int sem_res = sem_init(&i_map->up_queues[i].sem, 1, 1);

        if (sem_res == -1) {
            if (errno == ENOSYS) {
                printf("Process-shared semaphores not supported on this system.\n");
            } else {
                printf("Unknown error while initializing semaphore.\n");
            }

            return 1;
        }
    }
    
    for (int i=0; i<NUM_DOWN_QUEUES; i++) {
        // Initialize semaphore with 1, since nothing has to read or write immediately 
        int sem_res = sem_init(&i_map->down_queues[i].sem, 1, 1);

        if (sem_res == -1) {
            if (errno == ENOSYS) {
                printf("Process-shared semaphores not supported on this system.\n");
            } else {
                printf("Unknown error while initializing semaphore.\n");
            }

            return 1;
        }
    }
    // No need to initialize anything else since anonymous maps init with zeros


    // To avoid a race condition, we need to check PIDs
    int pid;
    pid_t ppid_before_fork = getpid();
    if ((pid = fork()) == -1) {
        // Fork failed
        printf("Fork failed!\n");
        return 1;
    } else if(pid == 0) {
        // Child process
        // Kill the child process if the parent dies
        int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (r == -1) { perror(0); return 1; }

        // Handle race condition for prctl
        if (getppid() != ppid_before_fork) {
            return 1;
        }

        start_cli(i_map);
        return 0;
    }

    // Parent process

    start_server((in_addr_t*) hostname->h_addr_list[0], arguments.port, i_map);

    return 0;
}