#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "aes.h"
#include "handler.h"
#include "msg_reader.h"
#include "ipc.h"
#include "messages.h"
#include "messaging.h"
#include "util.h"

size_t read_fragmented_message(uint64_t this_client, struct message_queues* i_map, struct message* start_message_copy, void* message_out, size_t message_size) {
    
    if (message_size <= sizeof(struct dynamic_part)) {
        // Special case, copy the part and return
        memcpy(&message_out, &start_message_copy->dynamic_part, message_size);
        return message_size;
    }

    // Copy the first part
    memcpy(&message_out, &start_message_copy->dynamic_part, sizeof(struct dynamic_part));

    size_t bytes_left = message_size - sizeof(struct dynamic_part);
    uint64_t prev_seq = start_message_copy->seq;

    // Loop through queues to find parts

    // TODO: Yes this is duplicated code sorry
    // This is what I get for programming in C


    // Loop through a few times just in case not everything was added
    // TODO: This is implemented poorly and has an edge case if fragments are somehow ordered backwards
    for (int a=0; a<MAX_FRAG_ATTEMPTS; a++) {
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

                if (!message->fragmented) {
                    continue;
                }

                if (message->seq == prev_seq+1) {
                    // This is the next message in the sequence, copy and mark as read

                    size_t bytes_to_read = sizeof(struct dynamic_part);

                    if (bytes_left < bytes_to_read) {
                        bytes_to_read = bytes_left;
                    }

                    memcpy(message_out + (message_size-bytes_left), &message->dynamic_part, bytes_to_read);

                    ClearBit(queue->bit_mask, m);

                    prev_seq++;
                    bytes_left -= bytes_to_read;
                }

                // All parts read!
                if (bytes_left == 0) {
                    sem_post(&queue->sem);
                    return message_size;
                }

            }

            sem_post(&queue->sem);
        }
    }
    return message_size - bytes_left;
}

void handle_frag_message(int sock, uint64_t this_client, void *message, size_t message_size, IPC_MESSAGE type, struct AES_ctx* ctx) {

    if (type == IPC_SYSTEM) {
        struct tc2_msg_preamble preamble;
        preamble.type = MSG_SYSTEM;
        preamble.len = message_size;

        write_encrypted_padded(sock, ctx, &preamble, sizeof(preamble));
        write_encrypted_padded(sock, ctx, message, message_size);
    } else {
        printf("Unknown message type %d", (int) type);
    }
}

void read_and_handle_messages(int sock, uint64_t this_client, struct message_queues* i_map, struct AES_ctx* ctx) {
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

            if (message->fragment_start) {
                // Start reading fragmented message
                size_t total_frag_size = message->total_size;

                void* message_out = malloc(total_frag_size);

                struct message message_copy;

                memcpy(&message_copy, message, sizeof(struct message));

                ClearBit(queue->bit_mask, m);
                
                sem_post(&queue->sem);

                size_t bytes_read = read_fragmented_message(this_client, i_map, &message_copy, message_out, total_frag_size);

                if (bytes_read < total_frag_size) {
                    printf(RED "[-] Could not read all parts of fragmented message!\n" RESET);
                    continue;
                }

                // Handle fragmented message
                IPC_MESSAGE type = message_copy.type;
                handle_frag_message(sock, this_client, message_out, total_frag_size, type, ctx);

                free(message_out);

                sem_wait(&queue->sem);

                continue;
            }
           
            if (message->fragmented) {
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

            IPC_MESSAGE type = message->type;

            if (type == IPC_PING) {
                printf("Ping received on client %ld\n", this_client);
            } else {
                printf("Unknown message type %d", (int) type);
            }
        }
    }
}

void handle(int sock, uint64_t client_id, struct message_queues* m_queue, struct AES_ctx* ctx) {
    printf("Starting handler for client %ld\n", client_id);

    // Keep track of client info here, send a copy to the CLI when it updates
    struct client_info info;
    info.ipc_id = client_id;
    info.num_caps = 0;

    // Make socket non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    bool reading_array = false;
    void* working_arr;
    uint32_t array_size;
    uint32_t arr_index = 0;
    TC2_MESSAGE_TYPE_ENUM arr_type;

    struct tc2_msg_preamble preamble;
    size_t paddded_size = sizeof(preamble) + AES_BLOCKLEN - (sizeof(preamble) % AES_BLOCKLEN);
    size_t bytes_left = paddded_size;
    char* temp_preamble_buffer = malloc(paddded_size);

    while (1) {
        read_and_handle_messages(sock, client_id, m_queue, ctx);

        // Non-blocking read preamble, then message
        int val = read(sock, ((void*)temp_preamble_buffer) + (paddded_size - bytes_left), bytes_left);

        // TODO: This will not always be how this is handled. In the future, we want to continue listening for the same client to reconnect.
        // This is mostly done on the cli/api side of things. We just want to retain client info even after it disconnects and if
        // a client with the same internal ID connects, we know it's the same so we just re-assign it to be the same.
        if (val == -1) {
            if (errno != EAGAIN) {
                // Error
                printf("Error while reading socket for client %ld. Errno %d\n", client_id, errno);
                free(temp_preamble_buffer);
                return;
            }
            // Non-blocking
            continue;
        } else if (val == 0) {
            // EOF
            printf("Client disconnected\n");
            free(temp_preamble_buffer);
            return;
        }

        bytes_left -= val;

        if (bytes_left == 0) {
            // Full preamble read, handle it.

            // TODO: Add tiny-HMAC to verify that packets were decrypted successfully?
            // Decrypt and copy over preamble
            AES_CBC_decrypt_buffer(ctx, (uint8_t*) temp_preamble_buffer, paddded_size);
            memcpy(&preamble, temp_preamble_buffer, sizeof(preamble));

            // Clean up and prep for next preamble
            free(temp_preamble_buffer);
            temp_preamble_buffer = malloc(paddded_size);
            bytes_left = paddded_size;
            

            size_t expected_len = 0;

            if (preamble.type == MSG_ARRAY) {
                // Handle starting array
                expected_len = sizeof(struct tc2_array);
            } else if (preamble.type == MSG_ARRAY_STOP) {
                expected_len = sizeof(struct tc2_array_stop);
            } else if (preamble.type == MSG_CAPABILITY) {
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
            size_t paddded_msg_size = expected_len + AES_BLOCKLEN - (expected_len % AES_BLOCKLEN);
            void* temp_msg_buffer = malloc(paddded_msg_size);
            size_t msg_bytes_left = paddded_msg_size;

            while (msg_bytes_left > 0) {
                // Probably want to time out somehow too
                int num_read = read(sock, temp_msg_buffer + (paddded_msg_size - msg_bytes_left), msg_bytes_left);

                if (num_read == -1) {
                    if (errno != EAGAIN) {
                        // Error
                        printf("Error while reading socket for client %ld. Errno %d\n", client_id, errno);
                        free(temp_msg_buffer);
                        free(temp_preamble_buffer);
                        return;
                    }
                    // Non-blocking
                    continue;
                } else if (num_read == 0) {
                    // EOF
                    printf("Client disconnected\n");
                    free(temp_msg_buffer);
                    free(temp_preamble_buffer);
                    return;
                }

                msg_bytes_left -= num_read;
            }

            // Message read. Parse
            void* message = malloc(expected_len);
            AES_CBC_decrypt_buffer(ctx, temp_msg_buffer, paddded_msg_size);
            memcpy(message, temp_msg_buffer, expected_len);
            free(temp_msg_buffer);

            if (preamble.type == MSG_ARRAY) {
                if (reading_array) {
                    printf("Error. Already reading array\n");
                    goto clean;
                }

                struct tc2_array *msg_arr = (struct tc2_array *)message;

                TC2_MESSAGE_TYPE_ENUM type = msg_arr->type;
                size_t msg_size;

                // Since types are compile time in C, there's no clean way to do this afaik

//----------------- Array Types ------------------
                if (type == MSG_CAPABILITY) {
                    msg_size = sizeof(struct tc2_capability);
                } else {
                    printf("Cannot create an array of type %d\n", type);
                    goto clean;
                }

                if (msg_arr->num_elements == 0) {
                    printf("Cannot create an array of size 0\n");
                    goto clean;
                }

                if (msg_arr->num_elements > MAX_ARR_ELEMENTS) {
                    printf("Array too large. Cannot create an array of size %d\n", msg_arr->num_elements);
                    goto clean;
                }

                reading_array = true;
                arr_type = type;
                working_arr = malloc(msg_size * msg_arr->num_elements);
                arr_index = 0;
                array_size = msg_arr->num_elements;

                printf("Array Started\n");
            } else if (preamble.type == MSG_ARRAY_STOP) {
                struct tc2_array_stop *msg_arr_stop = (struct tc2_array_stop *)message;

                if (!reading_array) {
                    printf("Cannot stop array that hasn't been started\n");
                    goto clean;
                }

                reading_array = false;

//----------------- Array Types ------------------
                if (arr_type == MSG_CAPABILITY) {
                    handle_arr_capability(sock, client_id, m_queue, &info,(struct tc2_capability *) working_arr, arr_index);
                } else {
                    printf("Cannot handle array of type %d. Something went wrong!\n", arr_type);
                    free(working_arr);
                    goto clean;
                }

                free(working_arr);                
            } else if (preamble.type == MSG_CAPABILITY) {
                struct tc2_capability *msg_cap = (struct tc2_capability *)message;

                if (reading_array) {
                    if (arr_type != MSG_CAPABILITY) {
                        printf("Array type not MSG_CAPABILITY. Is %d\n", arr_type);
                        goto clean;
                    }

                    // Store to array
                    if (arr_index >= array_size) {
                        printf("Array already full\n");
                        goto clean;
                    }

                    memcpy(&((struct tc2_capability *)working_arr)[arr_index], msg_cap, sizeof(struct tc2_capability));
                    arr_index++;

                    goto clean;
                }

                handle_capability(sock, client_id, m_queue, &info, msg_cap);
            } else {
                // This should be impossible at this point, but just in case
                printf("Cannot handle message type %d\n", preamble.type);
                goto clean;
            }

            clean: free(message);
        }

    }
}