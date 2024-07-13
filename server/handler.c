#include <stdio.h>
#include <unistd.h>
#include "handler.h"
#include "ipc.h"

void handle(int sock, struct message_queues* m_queue) {
    while (1) {
        char buf[16];
        read(sock, buf, 1);
        return;
    }
}