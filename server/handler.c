#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "handler.h"
#include "ipc.h"
#include "messages.h"
#include "util.h"

void handle_arr_capability(int sock, uint64_t client_id, struct message_queues* m_queue, struct client_info *info, struct tc2_capability cap_arr[], uint32_t num_elements) {
    if (m_queue == NULL) {
        printf(RED "Message queue is NULL while handling capability array.\n" RESET);
        return;
    }

    if (info == NULL) {
        printf(RED "Client info is NULL while handling capability array.\n" RESET);
        return;
    }

    if (cap_arr == NULL) {
        printf(RED "Capability array is NULL while handling capability array.\n" RESET);
        return;
    }

    bool info_changed = false;

    for (int c=0; c<num_elements; c++) {
        struct tc2_capability *cap = &cap_arr[c];


        // Check if capability is already in info
        for (int i=0; i<info->num_caps; i++) {
            if (cap->cap == info->capabilities[i]) {
                if (cap->add_rem == ADD) {
                    printf(YEL "Capability %d already present in client %ld\n" RESET, cap->cap, client_id);
                } else {
                    printf(YEL "Removing capabilities not yet supported\n" RESET);
                }
                goto next_cap;
            }
        }

        // Capability does not exist
        if (cap->add_rem == REMOVE) {
            printf(YEL "Removing capabilities not yet supported\n" RESET);
            goto next_cap;
        }

        // Add capability
        info->capabilities[info->num_caps] = cap->cap;
        info->num_caps++;

        info_changed = true;

        next_cap:;
    }

    if (info_changed) {
        // Send updated client info
        struct message msg;
        msg.client_id = client_id;
        msg.fragmented = false;
        msg.fragment_end = false;
        msg.type = IPC_CLIENT_INFO;
        memcpy(&msg.client_info_message, info, sizeof(struct client_info));

        ipc_send_message_up_blocking(m_queue, &msg);
    }
}

void handle_capability(int sock, uint64_t client_id, struct message_queues* m_queue, struct client_info *info, struct tc2_capability *cap) {

    if (m_queue == NULL) {
        printf(RED "Message queue is NULL while handling capability.\n" RESET);
        return;
    }

    if (info == NULL) {
        printf(RED "Client info is NULL while handling capability.\n" RESET);
        return;
    }

    if (cap == NULL) {
        printf(RED "Capability is NULL while handling capability.\n" RESET);
        return;
    }

    // Check if capability is already in info
    for (int i=0; i<info->num_caps; i++) {
        if (cap->cap == info->capabilities[i]) {
            if (cap->add_rem == ADD) {
                printf(YEL "Capability %d already present in client %ld\n" RESET, cap->cap, client_id);
            } else {
                printf(YEL "Removing capabilities not yet supported\n" RESET);
            }
            return;
        }
    }

    // Capability does not exist
    if (cap->add_rem == REMOVE) {
        printf(YEL "Removing capabilities not yet supported\n" RESET);
        return;
    }

    // Add capability
    info->capabilities[info->num_caps] = cap->cap;
    info->num_caps++;

    // Send updated client info
    struct message msg;
    msg.client_id = client_id;
    msg.fragmented = false;
    msg.fragment_end = false;
    msg.type = IPC_CLIENT_INFO;
    memcpy(&msg.client_info_message, info, sizeof(struct client_info));

    ipc_send_message_up_blocking(m_queue, &msg);
}