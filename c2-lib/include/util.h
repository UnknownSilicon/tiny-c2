#include <stdint.h>

#ifndef WINDOWS
#include <sys/time.h>
#include <sys/types.h>

// Set timeouts
// Returns 0 on success, 1 if RCVTIMEO fails, 2 if SNDTIMEO fails, and 3 if both fail
int set_timeout(int sock, struct timeval *timeout);
#endif

// Generate `size` random bytes with malloc. Remember to free!
uint8_t* rand_bytes(ssize_t size);


// Linked List

struct ll_node {
    struct ll_node* backward;
    struct ll_node* forward;
    void* data;  
};