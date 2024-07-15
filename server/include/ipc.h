#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include "cassert.h"

#ifndef IPC_H
#define IPC_H

// Defines the number of messages each queue can store
#define QUEUE_SIZE 128
CASSERT(!(QUEUE_SIZE % 8), queue_size_ipc);

// Defines the number of processes that can simultaneously write messages to the cli
#define NUM_UP_QUEUES 4

// Defines the number of processes that can simultaneously read messages from the cli
#define NUM_DOWN_QUEUES 1

// Defines the max size any given meessage can be
// Past this, it must be fragmented
// This is to prevent all messages from being unnecesarily large
#define MAX_MESSAGE_SIZE 1024

// Bit array operations for use in masked_queue
#define SetBit(A,k)     ( A[(k)/8] |= (1 << ((k)%8)) )
#define ClearBit(A,k)   ( A[(k)/8] &= ~(1 << ((k)%8)) )
#define TestBit(A,k)    ( A[(k)/8] & (1 << ((k)%8)) )

typedef enum IPC_MESSAGE {
    IPC_NULL, // No message, for marking empty regions
    IPC_INIT,
    IPC_DISCONNECT,
    IPC_PING
} IPC_MESSAGE;

struct client_info {
    uint64_t id;
};

struct empty_message {
    char dummy[1];
};

struct ping_message {
    char data[16];
};

struct message {
    uint64_t client_id;
    bool fragmented;
    bool fragment_end;
    IPC_MESSAGE type;
    union {
        struct empty_message init_message;
        struct empty_message disconnect_message;
        struct ping_message ping_message;
    };
};
// Enforce that the max size of message is below the hardcoded max
CASSERT(sizeof(struct message) <= MAX_MESSAGE_SIZE, message_size_ipc);

struct queue {
    sem_t sem;
    uint16_t num_elements;
    struct message messages[QUEUE_SIZE];
};

struct masked_queue {
    sem_t sem;
    uint8_t bit_mask[QUEUE_SIZE / 8];
    struct message messages[QUEUE_SIZE];
};

struct message_queues {
    struct queue up_queues[NUM_UP_QUEUES];
    struct masked_queue down_queues[NUM_DOWN_QUEUES];
};

void ipc_send_message_up_blocking(struct message_queues *queues, struct message *message);
void ipc_send_message_down_blocking(struct message_queues *queues, struct message *message);



#endif