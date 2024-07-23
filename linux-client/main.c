#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "aes.h"
#include "capabilities.h"
#include "messages.h"
#include "util.h"

// TODO: Dynamically generate this based on compiler flags, adding in capability files as needed
TC2_CAPABILITY_ENUM capabilities[] = { SYSTEM };
uint32_t NUM_CAPS = 1;

int main(int argc, char* argv[]) {
    srand(time(NULL));
    struct sockaddr_in sockaddr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(1234);

    struct hostent* hostname = gethostbyname("127.0.0.1");
    sockaddr.sin_addr.s_addr = *((in_addr_t*) hostname->h_addr_list[0]);

    connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    struct tc2_msg_init init_msg;

    // Encrypt ID with key
    struct AES_ctx ctx;

    char* iv = rand_bytes(16);

    uint8_t key[] = AES_KEY;

    AES_init_ctx_iv(&ctx, key, iv);
    memcpy(init_msg.iv, iv, 16);
    free(iv);

    uint8_t secret[] = CLIENT_SECRET;

    memcpy(init_msg.enc_id, secret, 32);
    AES_CBC_encrypt_buffer(&ctx, init_msg.enc_id, 32);
    

    struct tc2_msg_preamble preamble;
    preamble.type = INIT;
    preamble.len = sizeof(init_msg);

    write(sock, &preamble, sizeof(preamble));
    write(sock, &init_msg, sizeof(init_msg));

    char data[16];
    fgets(data, 8, stdin);
    write(sock, "1", 1);

    sleep(10);

    close(sock);

    return 0;
}