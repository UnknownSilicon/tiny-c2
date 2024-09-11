#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include "ipc.h"
#include "util.h"

// Global sequence counter
// Yes, this works with the multiprocessing
// No, this won't overflow
// Even if it was incremented every CPU cycle, it will take several million years to overflow
uint64_t next_seq = 0;

void ipc_send_message_up_blocking(struct message_queues *queues, struct message *message) {
    int i = 0;

    while (1) {
        struct queue *queue = &queues->up_queues[i];

        int res = sem_trywait(&queue->sem);

        // If we couldn't acquire this semaphore, try the next one
        if (res == -1) {
            i += 1;
            if (i == NUM_UP_QUEUES) {
                i = 0;
            }
            continue;
        }

        // Now, we have a queue

        int index = queue->num_elements;
        

        if (index == QUEUE_SIZE) {
            // This queue is full, release semaphore, try again.
            sem_post(&queue->sem);
            i += 1;
            if (i == NUM_UP_QUEUES) {
                i = 0;
            }
            continue;
        }

        // There's room in this queue
        memcpy(&queue->messages[index], message, sizeof(struct message));
        queue->num_elements += 1;

        // Release semaphore and exit
        sem_post(&queue->sem);
        return;
    }
}

void ipc_send_message_down_blocking(struct message_queues *queues, struct message *message) {
    int i = 0;

    while (1) {
        struct masked_queue *queue = &queues->down_queues[i];

        int res = sem_trywait(&queue->sem);

        // If we couldn't acquire this semaphore, try the next one
        if (res == -1) {
            i += 1;
            if (i == NUM_DOWN_QUEUES) {
                i = 0;
            }
            continue;
        }

        uint32_t free = 0;
        bool found = false;

        for (uint32_t i=0; i<QUEUE_SIZE; i++) {
            // Find the first free slot
            if (!TestBit(queue->bit_mask, i)) {
                // Free
                free = i;
                found = true;
                break;
            }
        }
        

        if (!found) {
            // This queue is full, release semaphore, try again.
            sem_post(&queue->sem);
            i += 1;
            if (i == NUM_DOWN_QUEUES) {
                i = 0;
            }
            continue;
        }

        // There's room in this queue
        memcpy(&queue->messages[free], message, sizeof(struct message));
        SetBit(queue->bit_mask, free);

        // Release semaphore and exit
        sem_post(&queue->sem);
        return;
    }
}

// I'm sorry for this...
void ipc_send_dynamic_message_down_blocking(struct message_queues *queues, uint64_t client_id, IPC_MESSAGE type, void* full_data, uint64_t data_len) {
    // Here, I need to construct individual messages containing the info needed
    struct message message;
    message.client_id = client_id;
    message.fragment_start = true;
    message.fragmented = true;
    message.total_size = data_len;
    message.type = type;

    uint16_t num_messages;
    uint16_t last_size;
    uint32_t block_len;

    if (type == IPC_SYSTEM) {
        block_len = (sizeof (struct dynamic_part));
        last_size = data_len % block_len;
        num_messages = (data_len - last_size) / block_len;
    } else {
        printf(RED "Cannot send type %d as dynamic sized\n" RESET, type);
        return;
    }

    for (int i=0; i<num_messages; i++) {
        void* data_loc = full_data + (block_len * i);

        message.seq = next_seq++;
        
        if (type == IPC_SYSTEM) {
            memcpy(&message.dynamic_part, data_loc, block_len);
        } else {
            printf(RED "Cannot send type %d as dynamic sized\n" RESET, type);
            return;
        }

        ipc_send_message_down_blocking(queues, &message);
        message.fragment_start = false;
    }

    if (last_size > 0) {
        // Send final message
        message.seq = next_seq++;

        void* data_loc = full_data + (block_len * num_messages);

        if (type == IPC_SYSTEM) {
            memcpy(&message.dynamic_part, data_loc, last_size);
        } else {
            printf(RED "Cannot send type %d as dynamic sized\n" RESET, type);
            return;
        }

        ipc_send_message_down_blocking(queues, &message);
    }
}