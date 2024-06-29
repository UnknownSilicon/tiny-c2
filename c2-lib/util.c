#include <errno.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"

// Set timeouts
// Returns 0 on success, 1 if RCVTIMEO fails, 2 if SNDTIMEO fails, and 3 if both fail
int set_timeout(int sock, struct timeval *timeout) {
    int res = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)timeout, sizeof (struct timeval)) < 0) {
        res |= 1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)timeout, sizeof (struct timeval)) < 0) {
        res |= 2;
    }
    return res;
}


// Generate `size` random bytes with malloc. Remember to free!
uint8_t* rand_bytes(ssize_t size) {
    uint8_t* mem = malloc(size);

    for (int i=0; i < size; i++) {
        mem[i] = rand() % 255;
    }

    return mem;
}