#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include "cassert.h"

#ifndef IPC_H
#define IPC_H

// Defines the number of messages each queue can store
#define QUEUE_SIZE 100

// Defines the number of processes that can simultaneously read/write a message
#define NUM_QUEUES 4

// Defines the max size any given meessage can be
// Past this, it must be fragmented
// This is to prevent all messages from being unnecesarily large
#define MAX_MESSAGE_SIZE 1024

typedef enum IPC_MESSAGE {
    IPC_INIT
} IPC_MESSAGE;

struct client_info {
    uint64_t id;
};

struct init_message {
    char dummy[16];
};

struct message {
    uint64_t client_id;
    bool fragmented;
    bool fragment_end;
    IPC_MESSAGE type;
    union {
        struct init_message init_message;
    };
};
// Enforce that the max size of message is below the hardcoded max
CASSERT(sizeof(struct message) <= MAX_MESSAGE_SIZE, ipch);

struct queue {
    sem_t sem;
    uint16_t num_elements;
    struct message messages[QUEUE_SIZE];
};

struct message_queues {
    struct queue queues[NUM_QUEUES];
};

void ipc_send_message_blocking(struct message_queues *queues, struct message *message);

#endif