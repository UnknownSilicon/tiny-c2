#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "handler.h"
#include "ipc.h"
#include "messages.h"
#include "util.h"

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

    // Make socket non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    bool reading_array = false;
    void* working_arr;
    TC2_MESSAGE_TYPE_ENUM arr_type;

    struct tc2_msg_preamble preamble;
    size_t bytes_left = sizeof(preamble);
    while (1) {
        read_and_handle_messages(client_id, m_queue);

        // Non-blocking read preamble, then message
        int val = read(sock, ((void*)&preamble) + (sizeof(preamble) - bytes_left), bytes_left);

        // TODO: This will not always be how this is handled. In the future, we want to continue listening for the same client to reconnect.
        // This is mostly done on the cli/api side of things. We just want to retain client info even after it disconnects and if
        // a client with the same internal ID connects, we know it's the same so we just re-assign it to be the same.
        if (val == -1) {
            // Error
            printf("Error while reading socket for client %ld\n", client_id);
            return;
        } else if (val == 0) {
            // EOF
            printf("Client disconnected\n");
            return;
        }

        bytes_left -= val;

        if (bytes_left == 0) {
            // Full preamble read, handle it.

            size_t expected_len = 0;

            if (preamble.type == ARRAY) {
                // Handle starting array
                expected_len = sizeof(struct tc2_array);
            } else if (preamble.type == ARRAY_STOP) {
                expected_len = sizeof(struct tc2_array_stop);
            } else if (preamble.type == CAPABILITY) {
                expected_len = sizeof(struct tc2_capability);
            } else {
                printf("Cannot handle message type %d\n", preamble.type);
                continue;
            }

            if (preamble.len != expected_len) {
                printf("Unepxected length in preamble. Expected %ld, got %d\n", expected_len, preamble.len);
                continue;
            }

            // Read the actual message, non-blocking
            void* message = malloc(expected_len);
            size_t msg_bytes_left = expected_len;

            while (1) {
                // Probably want to time out somehow too
                int num_read = read(sock, message + (expected_len - msg_bytes_left), msg_bytes_left);

                if (num_read == -1) {
                    // Error
                    printf("Error while reading socket for client %ld\n", client_id);
                    free(message);
                    return;
                } else if (num_read == 0) {
                    // EOF
                    printf("Client disconnected\n");
                    free(message);
                    return;
                }

                msg_bytes_left -= num_read;

                if (msg_bytes_left == 0) {
                    break;
                }
            }

            // Message read. Parse

            if (preamble.type == ARRAY) {
                if (reading_array) {
                    printf("Error. Already reading array\n");
                    free(message);
                    continue;
                }

                struct tc2_array *msg_arr = (struct tc2_array *)message;

                TC2_MESSAGE_TYPE_ENUM type = msg_arr->type;
                size_t msg_size;

                // Since types are compile time in C, there's no clean way to do this afaik
                if (type == CAPABILITY) {
                    msg_size = sizeof(struct tc2_capability);
                } else {
                    printf("Cannot create an array of type %d\n", type);
                    free(message);
                    continue;
                }

                if (msg_arr->num_elements == 0) {
                    printf("Cannot create an array of size 0\n");
                    free(message);
                    continue;
                }

                reading_array = true;
                arr_type = type;
                working_arr = malloc(msg_size * msg_arr->num_elements);

                printf("Array Started\n");
            } else if (preamble.type == ARRAY_STOP) {
                struct tc2_array_stop *msg_arr_stop = (struct tc2_array_stop *)message;
            } else if (preamble.type == CAPABILITY) {
                struct tc2_capability *msg_cap = (struct tc2_capability *)message;
                
            } else {
                // This should be impossible at this point, but just in case
                printf("Cannot handle message type %d\n", preamble.type);
                free(message);
                continue;
            }

            free(message);
        }

    }
}