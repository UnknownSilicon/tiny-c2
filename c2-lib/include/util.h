#ifndef UTIL_H
#define UTIL_H 1
#include <stdint.h>

#define DEBUG 1

// ANSI escape sequences for color
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#ifndef WINDOWS
#include <sys/time.h>
#include <sys/types.h>
// Set timeouts
// Returns 0 on success, 1 if RCVTIMEO fails, 2 if SNDTIMEO fails, and 3 if both fail
int set_timeout(int sock, struct timeval *timeout);
#endif

#define SIZEOF_ARR(arr) (sizeof(arr) / sizeof(*arr))

// Generate `size` random bytes with malloc
void rand_bytes(uint8_t loc[], size_t size);


// Linked List

struct ll_node {
    struct ll_node* backward;
    struct ll_node* forward;
    void* data;  
};
#endif