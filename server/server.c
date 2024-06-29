#include <stdio.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "aes.h"
#include "messages.h"
#include "server.h"

void handle_sigchld(int signum) {
    wait(NULL);
}

void start_server(in_addr_t* host, short port) {
    struct sockaddr_in sockaddr;
    int pid;

    signal(SIGCHLD, handle_sigchld);

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = *host;

    bind(serv_sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr));

    listen(serv_sock, 10);

    struct timeval tv;
    tv.tv_sec = 5; // 5 second timeout
    tv.tv_usec = 0;

    struct timeval no_timeout;
    no_timeout.tv_sec = 0;
    no_timeout.tv_usec = 0;


    printf("Server started\n");

    while(1) {
        fflush(stdout);
        struct sockaddr_in client_sockaddr;

        int sin_size = sizeof(struct sockaddr);
        int connfs = accept(serv_sock, (struct sockaddr *)&client_sockaddr, &sin_size);

        // Set socket timeout, at least during initialization
        // This prevents clients from completely locking up the init process
        setsockopt(connfs, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(connfs, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

        printf("Connection acquired. Client Addr: %s\n", inet_ntoa(client_sockaddr.sin_addr));

        // After acquiring connection, perform init sequence
        // If client successfully authenticates, fork and proceed
        // Otherwise, kill the connection
        // TODO: This isn't the best way to do this
        // It's still possible to DOS the server in this case

        struct tc2_msg_preamble preamble;

        read(connfs, &preamble, sizeof(preamble));

        printf("Received preamble. Type %d\n", preamble.type);

        if (preamble.type != INIT) {
            printf("Received unexpected type. Closing...\n");
            close(connfs);
            continue;
        }

        if (INIT_STYLE == FIXED) {
            if (preamble.len != sizeof(struct tc2_msg_init)) {
                printf("Received unexpected message length. INIT is Fixed message length of %ld, but got %d\n", sizeof(struct tc2_msg_init), preamble.len);
                close(connfs);
                continue;
            }
        }

        // Read init message, check key
        struct tc2_msg_init init_msg;

        read(connfs, &init_msg, preamble.len);

        printf("Got data: %s\n", init_msg.key);

        char* expected_key = AES_KEY;

        fflush(stdout);

        if (strncmp(init_msg.key, expected_key, 64) != 0) {
            printf("Unexpected key! Quitting\n");
            close(connfs);
            continue;
        }

        printf("Key valid. Proceeding...\n");

        // To avoid a race condition, we need to check PIDs
        pid_t ppid_before_fork = getpid();
        if ((pid = fork()) == -1) {
            // Fork failed
            printf("Fork failed!\n");
            close(connfs);
            continue;
        } else if(pid > 0) {
            // Parent process
            // Potentially use this to count active child processes
            // Useful to ensure not too many clients establish connections at once
            // And DOS the server
            printf("Hello from parent process\n");
            fflush(stdout);
            close(connfs);
            continue;
        }
        // Kill the child process if the parent dies
        int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (r == -1) { perror(0); exit(1); }

        // Handle race condition for prctl
        if (getppid() != ppid_before_fork) {
            exit(1);
        }

        // Child process
        printf("Hello from child process\n");

        // At this point, the client has authenticated and we're in the child process,
        // so we can disable timeout.
        setsockopt(connfs, SOL_SOCKET, SO_RCVTIMEO, (const char*)&no_timeout, sizeof no_timeout);
        setsockopt(connfs, SOL_SOCKET, SO_SNDTIMEO, (const char*)&no_timeout, sizeof no_timeout);

        fflush(stdout);

        //char buf[100] = "abc";
        //write(connfs, buf, strlen(buf));

        exit(0);
    }
}