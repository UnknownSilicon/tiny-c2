#include <argp.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <sys/prctl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cli.h"
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

        start_cli();
        return 0;
    }

    // Parent process

    start_server((in_addr_t*) hostname->h_addr_list[0], arguments.port);


    /*
    Note: It might be better for the CLI to Also be a child process and the parent to not really do much, or be the server process.
    The server process makes sense since it's already creating children, so if it handles memory map creation, we can mark memory
    as anonymous.
    IPC method:

    Shared memory.
    (Questionable part): First, the grandparent process, this one, creates a shared memory region.
    This region will contain temporary references to other memory regions, created by the server handler.
    This way, each child has their own message buffer.
    The catch here is there needs to be a lock/semaphore/synchronization solution so there isn't a
    race condition of updating this.
    For now, since there's just one server process, the parent process and server process will be using this.

    On successful connection, the server process locks the structure if it can
    (or waits until it can obtain it. Need to make sure the server doesn't hold it for too long).
    It will then create a memory region for the child process and store the pointer to it in this connection
    map. This just needs to be temporary and can be cleared once the parent process sees it.

    When the cli process does Anything, it needs to go and read the connections block, store it locally, increasing
    local memory as needed, and clear the block. Now, the cli process has a reference to the child's shared memory.
    The child process is mostly self-sustaining, as it can do the basic comms back and forth, but the cli is used to
    issue commands to the child using this individual shared memory block.
    This Also needs a locking mechanism, but similarly it should be fine as the first thing each side does when reading memory is
    copying it to local memory.
    
    */

    return 0;
}