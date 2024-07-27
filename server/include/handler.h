#include <stdint.h>
#include "ipc.h"
#include "messages.h"

void handle_arr_capability(int sock, uint64_t client_id, struct message_queues* m_queue, struct tc2_capability cap_arr[], uint32_t num_elements);

void handle_capability(int sock, uint64_t client_id, struct message_queues* m_queue, struct tc2_capability *cap);