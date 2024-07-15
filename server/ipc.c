#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include "ipc.h"

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