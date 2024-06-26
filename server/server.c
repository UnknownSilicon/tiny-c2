#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "messages.h"
#include "server.h"

void start_server(in_addr_t* host, short port) {
    struct sockaddr_in sockaddr;

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = *host;

    bind(serv_sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr));

    listen(serv_sock, 10);

    printf("Server started\n");

    while(1) {
        struct sockaddr_in client_sockaddr;

        int sin_size = sizeof(struct sockaddr);
        int connfs = accept(serv_sock, (struct sockaddr *)&client_sockaddr, &sin_size);

        printf("Connection acquired. Client Addr: %s\n", inet_ntoa(client_sockaddr.sin_addr));

        // Here, fork

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

        struct tc2_msg_init init_msg;

        read(connfs, &init_msg, preamble.len);

        printf("Got data: %s\n", init_msg.key);

        fflush(stdout);

        //char buf[100] = "abc";
        //write(connfs, buf, strlen(buf));

        close(connfs);
    }
}