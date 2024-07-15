#include <stdint.h>
#include "ipc.h"

void handle(int sock, uint64_t client_id, struct message_queues* conn_data);