#include <sys/time.h>
#include <sys/types.h>

// Set timeouts
// Returns 0 on success, 1 if RCVTIMEO fails, 2 if SNDTIMEO fails, and 3 if both fail
int set_timeout(int sock, struct timeval *timeout);