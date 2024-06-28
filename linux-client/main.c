#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "messages.h"


int main(int argc, char* argv[]) {
    struct sockaddr_in sockaddr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(1234);

    struct hostent* hostname = gethostbyname("127.0.0.1");
    sockaddr.sin_addr.s_addr = *((in_addr_t*) hostname->h_addr_list[0]);

    connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    struct tc2_msg_init init_msg;
    strncpy(init_msg.key, "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\x00", 64);
    

    struct tc2_msg_preamble preamble;
    preamble.type = INIT;
    preamble.len = sizeof(init_msg);

    write(sock, &preamble, sizeof(preamble));
    write(sock, &init_msg, sizeof(init_msg));

    close(sock);

    return 0;
}