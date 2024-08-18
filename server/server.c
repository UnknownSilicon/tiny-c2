#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "aes.h"
#include "msg_reader.h"
#include "ipc.h"
#include "messages.h"
#include "server.h"
#include "util.h"

void handle_sigchld(int signum) {
    wait(NULL);
}

int server_sock = 0;

void stop_server(int sig) {
    if (server_sock != -1) {
        close(server_sock);
    }

    exit(1);
}


void start_server(in_addr_t* host, short port, struct message_queues* i_map) {
    struct sockaddr_in sockaddr;
    int pid;

    signal(SIGCHLD, handle_sigchld);
    signal(SIGTERM, stop_server);
    signal(SIGKILL, stop_server);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock == -1) {
        printf("Unable to create server socket\n");
        exit(1);
    }

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = *host;

    bind(server_sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr));

    listen(server_sock, 10);

    struct timeval tv;
    tv.tv_sec = 5; // 5 second timeout
    tv.tv_usec = 0;

    struct timeval no_timeout;
    no_timeout.tv_sec = 0;
    no_timeout.tv_usec = 0;


    printf("Server started\n");

    uint64_t next_id = 1;

    while(1) {
        struct sockaddr_in client_sockaddr;

        int sin_size = sizeof(struct sockaddr);
        int connfs = accept(server_sock, (struct sockaddr *)&client_sockaddr, (socklen_t*) &sin_size);

        if (connfs == -1) {
            printf("Unable to accept socket\n");
            exit(1);
        }

        if (next_id == ULONG_MAX) {
            printf("Unable to initialize a new ID. Cannot create new connections\n");
            close(connfs);
            exit(1);
        }


        // Set socket timeout, at least during initialization
        // This prevents clients from completely locking up the init process
        int to_res = set_timeout(connfs, &tv);
        if (to_res != 0) {
            printf("Unable to set socket timeout %d\n", to_res);
        }

        printf("Connection acquired. Client Addr: %s\n", inet_ntoa(client_sockaddr.sin_addr));

        // After acquiring connection, perform init sequence
        // If client successfully authenticates, fork and proceed
        // Otherwise, kill the connection
        // TODO: This isn't the best way to do this
        // It's still possible to DOS the server in this case

        struct tc2_msg_preamble preamble;

        int64_t bytes_read = read(connfs, &preamble, sizeof(preamble));
        if ((bytes_read == -1) || bytes_read < sizeof(preamble)) {
            printf("Timeout while reading preamble\n");
            close(connfs);
            continue;
        }

        printf("Received preamble. Type %d\n", preamble.type);

        if (preamble.type != MSG_INIT) {
            printf("Received unexpected type. Closing...\n");
            close(connfs);
            continue;
        }

        if (preamble.len != sizeof(struct tc2_msg_init)) {
            printf("Received unexpected message length. MSG_INIT is Fixed message length of %ld, but got %d\n", sizeof(struct tc2_msg_init), preamble.len);
            close(connfs);
            continue;
        }

        // Read init message, check key
        struct tc2_msg_init init_msg;

        bytes_read = read(connfs, &init_msg, preamble.len);
        if ((bytes_read == -1) || bytes_read < preamble.len) {
            printf("Timeout while reading init\n");
            close(connfs);
            continue;
        }

        printf("Got data\n");

        // Decrypt message
        struct AES_ctx ctx;
        uint8_t key[] = AES_KEY;
        AES_init_ctx_iv(&ctx, key, init_msg.iv);
        AES_CBC_decrypt_buffer(&ctx, init_msg.enc_id, 32);

        uint8_t expected_secret[] = CLIENT_SECRET;

        if (memcmp(init_msg.enc_id, expected_secret, 32) != 0) {
            printf("Unexpected key! Quitting\n");
            close(connfs);
            continue;
        }

        printf("Key valid. Proceeding...\n");

        uint64_t curr_id = next_id;
        next_id += 1;

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

        // At this point, the client has authenticated and we're in the child process,
        // so we can disable timeout.
        to_res = set_timeout(connfs, &no_timeout);
        if (to_res != 0) {
            printf("Unable to set socket timeout %d\n", to_res);
        }

        struct empty_message init_message;
        memset(&init_message, 0, sizeof(init_message));

        struct message message;
        message.client_id = curr_id;
        message.fragmented = false;
        message.fragment_end = false;
        message.type = IPC_INIT;
        message.init_message = init_message;

        ipc_send_message_up_blocking(i_map, &message);


        fflush(stdout);

        // Start handler
        handle(connfs, curr_id, i_map, &ctx);

        // When handler exits, send disconnect message
        close(connfs);

        struct empty_message dc_message_empty;
        memset(&dc_message_empty, 0, sizeof(dc_message_empty));

        struct message dc_message;
        dc_message.client_id = curr_id;
        dc_message.fragmented = false;
        dc_message.fragment_end = false;
        dc_message.type = IPC_DISCONNECT;
        dc_message.disconnect_message = dc_message_empty;

        ipc_send_message_up_blocking(i_map, &dc_message);

        printf("Closed\n");
        exit(0);
    }

    close(server_sock);
    server_sock = -1;
}