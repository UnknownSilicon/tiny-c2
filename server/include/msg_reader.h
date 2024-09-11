#include <stdint.h>
#include "ipc.h"

#define MAX_ARR_ELEMENTS 1024
#define MAX_FRAG_ATTEMPTS 4

void handle(int sock, uint64_t client_id, struct message_queues* conn_data, struct AES_ctx* ctx);