#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include "ipc.h"


/*
I still need to implement a method for the CLI to send messages Back to the clients, as right now only the clients send messages to the CLI. 
It would probably be best to leave these in separate message queues. Maybe have 3 for Client -> Server and 1 for Server -> Client
*/



void ipc_send_message_blocking(struct message_queues *queues, struct message *message) {
    int i = 0;

    while (1) {
        struct queue *queue = &queues->queues[i];

        int res = sem_trywait(&queue->sem);

        // If we couldn't acquire this semaphore, try the next one
        if (res == -1) {
            i += 1;
            if (i == NUM_QUEUES) {
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
            if (i == NUM_QUEUES) {
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