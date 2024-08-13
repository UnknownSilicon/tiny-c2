#include <stdbool.h>
#include <stdio.h>
#include "aes.h"
#include "messaging.h"
#include "util.h"

#ifdef DEBUG
#include <errno.h>
#endif

bool write_encrypted_padded(int sock, struct AES_ctx* ctx, void* data, size_t data_len) {
    size_t padding_size = data_len % AES_BLOCKLEN;
    size_t total_size = data_len + padding_size;

    char* temp_data = malloc(total_size);
    
    if (temp_data == NULL) {
        #ifdef DEBUG
        printf("Malloc failed. Errno %d\n", errno);
        #endif
        return false;
    }

    memcpy(temp_data, data, data_len);

    // Insert padding after data
    rand_bytes((uint8_t*) (temp_data + data_len), padding_size);

    AES_CBC_encrypt_buffer(ctx, (uint8_t*) temp_data, total_size);

    size_t bytes_left = total_size;
    // Send encrypted buffer
    while (bytes_left > 0) {
        ssize_t bytes_written = write(sock, temp_data + (total_size - bytes_left), bytes_left);
        if (bytes_written == -1) {
            #ifdef DEBUG
            printf("Write failed. Errno %d\n", errno);
            #endif
            return false;
        }

        bytes_left -= bytes_written;
    }

    return true;
}