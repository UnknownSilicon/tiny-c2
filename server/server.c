#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"

void start_server(in_addr_t* host, short port) {
    struct sockaddr_in sockaddr;

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = *host;

    bind(serv_sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr));

    listen(serv_sock, 10);

    printf("Server started");

    while(1) {
        int connfs = accept(serv_sock, NULL, NULL);

        // Here, fork

        char buf[100] = "abc";

        write(connfs, buf, strlen(buf));

        close(connfs);
    }
}