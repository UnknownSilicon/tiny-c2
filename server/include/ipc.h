#include <semaphore.h>
#include <stdint.h>

#ifndef IPC_H
#define IPC_H

#define NUM_CONNS 100

struct init_map {
    sem_t sem;
    uint16_t curr_conn;
    struct connection *conns[NUM_CONNS];
};

struct connection {
    sem_t sem;
    uint64_t id;
};
#endif