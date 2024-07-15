#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "handler.h"
#include "ipc.h"

/*
The problem with messages in this direction is the fact that any client can read any portion of messages.
Those messages must be removed, but the remaining messages must be untouched.
I think the best way here would be to simply have a bit array showing which message slots are taken
*/


void read_and_handle_messages(uint64_t this_client, struct message_queues* i_map) {
    struct message message_temp[QUEUE_SIZE];
    int message_index = 0;

    for (int i=0; i<NUM_DOWN_QUEUES; i++) {
        struct masked_queue* queue = &i_map->down_queues[i];

        // Lock queue and find relevant messages
        sem_wait(&queue->sem);
        
        for (int m=0; m<QUEUE_SIZE; m++) {
            // Only test messages that exist
            if (!TestBit(queue->bit_mask, m)) {
                continue;
            }

            struct message* message = &queue->messages[m];

            if (message->client_id != this_client) {
                // Not this client's message. Ignore it
                continue;
            }

            // Copy message and clear from queue
            memcpy(&message_temp[message_index], message, sizeof(struct message));

            message_index += 1;
            ClearBit(queue->bit_mask, m);
        }

        sem_post(&queue->sem);


        // Handle messages

        for (int m_num=0; m_num<message_index; m_num++) {
            struct message* message = &message_temp[m_num];

            if (message->fragmented || message->fragment_end) {
                printf("Fragmentation not yet implemented!\n");
                continue;
            }

            IPC_MESSAGE type = message->type;

            if (type == IPC_PING) {
                printf("Ping received on client %ld\n", this_client);
            } else {
                printf("Unknown message type %d", (int) type);
            }
        }
    }

    
}

void handle(int sock, uint64_t client_id, struct message_queues* m_queue) {
    printf("Starting handler for client %ld\n", client_id);
    while (1) {
        char buf[16];
        read(sock, buf, 1);
        read_and_handle_messages(client_id, m_queue);
        return;
    }
}