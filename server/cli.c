#include <limits.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cli.h"
#include "ipc.h"
#include "util.h"

static const char *HELP_TEXT =  MAG "Commands:\n" RESET
                                "\n"
                                "help - Display this text\n"
                                "exit - Exit the C2\n"
                                "list - List connected C2 clients\n"
                                "ping handler_id - Ping the internal client's handler for debugging\n"
                                "session handler_id - Set the provided handler as the current handler for other commands\n"
                                "capabilities - List the capabilities of the selected handler\n"
                                "caps - Short hand for capabilities\n"
                                "system command - Send a command to the selected handler"
                                "\n";

int str_eq(char* part, const char* literal) {
    size_t len = strlen(literal);

    return strncmp(part, literal, len) == 0;
}

// Returns NULL if client not found
struct ll_node* find_client(struct ll_node* root, uint64_t client_id) {

    if (root == NULL) {
        printf(RED "Root is null when finding client!\n" RESET);
        exit(1);
    }

    struct ll_node* next_node = root->forward;

    if (next_node == NULL) {
        printf(RED "Something broke! next_node is NULL\n" RESET);
        exit(1);
    }

    bool found_client = false;
    struct client_info* client_info = NULL;

    while (next_node->data != NULL) {
        client_info = (struct client_info*) next_node->data;
    
        if (client_info->ipc_id == client_id) {
            return next_node;
        }

        next_node = next_node->forward;
        
        if (next_node == NULL) {
            printf(RED "Something broke! next_node is NULL\n" RESET);
            exit(1);
        }
    }

    return NULL;
}

void parse_and_call(struct message_queues* i_map, char* input, struct ll_node* conn_root, struct client_info **selected_session) {
    char* saveptr;
    char* tok = strtok_r(input, " \n", &saveptr);

    if (tok == NULL) {
        return;
    }

    if (conn_root == NULL) {
        return;
    }

    if (str_eq(tok, "help")) {
        printf("%s", HELP_TEXT);
    } else if (str_eq(tok, "exit")) {
        if (kill(getppid(), SIGTERM) != 0) {
            // TODO: In parent, handle SIGTERM and actually close socket
            kill(getppid(), SIGKILL);
        }

        sleep(5);
        printf(RED "Couldn't exit after 5 seconds. Sending SIGKILL..." RESET);
        kill(getppid(), SIGKILL);
        exit(0);
    } else if (str_eq(tok, "list")) {
        printf(GRN "Current connections:\n" RESET);

        struct ll_node* next_node = conn_root->forward;

        if (next_node == NULL) {
            printf(RED "Something broke! next_node is NULL" RESET);
            exit(1);
        }

        while (next_node->data != NULL) {
            struct client_info* client_info = (struct client_info*) next_node->data;
        
            printf("Client %ld\n", client_info->ipc_id);

            next_node = next_node->forward;
        }
    } else if (str_eq(tok, "ping")) {
        // Get second argument
        tok = strtok_r(NULL, " \n", &saveptr);

        if (tok == NULL) {
            printf(RED "Missing parameter <clientid:ulong>\n" RESET);
            return;
        }

        uint64_t ul = strtoul(tok, NULL, 10);

        if (ul == 0 || ul == ULONG_MAX) {
            printf(RED "Invalid parameter. Expected unsigned long\n" RESET);
            return;
        }

        // Check if client exists.

        struct ll_node* node = find_client(conn_root, ul);

        if (node == NULL) { 
            printf(RED "Could not find client %ld\n" RESET, ul);
            return;
        }

        // ul is client ID
        struct ping_message ping_message;
        memset(&ping_message, 0, sizeof(ping_message));

        struct message message;
        message.client_id = ul;
        message.fragmented = false;
        message.fragment_end = false;
        message.type = IPC_PING;
        message.ping_message = ping_message;

        ipc_send_message_down_blocking(i_map, &message);

        printf(GRN "Ping sent to client %ld\n" RESET, ul);
    } else if (str_eq(tok, "session")) { 
        // Get second argument
        tok = strtok_r(NULL, " \n", &saveptr); // TODO: Deduplicate this code

        if (tok == NULL) {
            printf(RED "Missing parameter <clientid:ulong>\n" RESET);
            return;
        }

        uint64_t ul = strtoul(tok, NULL, 10);

        if (ul == 0 || ul == ULONG_MAX) {
            printf(RED "Invalid parameter. Expected unsigned long\n" RESET);
            return;
        }

        // Check if client exists.

        struct ll_node* node = find_client(conn_root, ul);

        if (node == NULL) {
            printf(RED "Could not find client %ld\n" RESET, ul);
            return;
        }

        struct client_info* client_info = (struct client_info*) node->data;

        if (client_info == NULL) {
            printf(RED "Something broke! client_info is NULL\n" RESET);
            exit(1);
        }

        *selected_session = client_info;

        printf(GRN "Selected client %ld\n" RESET, ul);
    } else if (str_eq(tok, "caps") || str_eq(tok, "capabilities")) {
        struct client_info* curr_client = *selected_session;

        if (curr_client == NULL) {
            printf(YEL "No session selected. Use" BLU " session " YEL "to select\n" RESET);
            return;
        }

        printf(GRN "Client Capabilities:\n" RESET);

        for (int c=0; c<curr_client->num_caps; c++) {
            TC2_CAPABILITY_ENUM cap = curr_client->capabilities[c];

            printf("%s\n", CAPABILITY_STRING[cap]);
        }

        printf("\n");
    } else if (str_eq(tok, "system")) {
        struct client_info* curr_client = *selected_session;

        if (curr_client == NULL) {
            printf(YEL "No session selected. Use" BLU " session " YEL "to select\n" RESET);
            return;
        }

        tok = strtok_r(NULL, "\n", &saveptr);

        if (tok == NULL) {
            printf(RED "Missing parameter <command>\n" RESET);
            return;
        }

        // tok is command. Send to client

        
        
    } else {
        printf(RED "Unknown command\n" RESET);
    }
}

void start_cli(struct message_queues* i_map) {

    struct client_info *selected_session = NULL;

    // Store a linked list of connections
    // Root is defined as the node with NULL data
    struct ll_node* root = malloc(sizeof (struct ll_node));
    root->backward = root;
    root->forward = root;
    root->data = NULL;

    struct ll_node* head = root;

    struct message message_temp[QUEUE_SIZE];

    char input[100];

    while (1) {

        if (selected_session != NULL) {
            printf(BLU "%ld" RESET " >> ", selected_session->ipc_id);
        } else {
            printf(">> ");
        }

        if (fgets(input, 100, stdin) == NULL) {
            printf(RED "Error reading input.\n" RESET);
            continue;
        }

        // Read message queues, then handle command
        for (int i=0; i<NUM_UP_QUEUES; i++) {
            struct queue* queue = &i_map->up_queues[i];

            // Lock queue and copy messages
            sem_wait(&queue->sem);

            // TODO: Potential optimization. Only copy memory equal to number of messages
            // Need to be careful about alignment
            memcpy(message_temp, &queue->messages, sizeof(message_temp));

            int temp_num = queue->num_elements;

            queue->num_elements = 0;
            // We don't need to actually remove the old messages

            // Unlock queue and handle messages
            sem_post(&queue->sem);

            for (int m=0; m<temp_num; m++) {
                struct message* message = &message_temp[m];

                if (message->fragmented || message->fragment_end) {
                    printf(RED "Fragmentation not yet implemented!\n" RESET);
                    continue;
                }

                uint64_t client_id = message->client_id;
                IPC_MESSAGE type = message->type;

                if (type == IPC_INIT) {
                    // Add connection to list

                    // TODO: Check if client ID is already connected. Idk if it's realistically easy to kill that new connection though.
                    struct ll_node* new_node = malloc(sizeof (struct ll_node));
                    new_node->backward = head;
                    struct ll_node* next = head->forward;
                    new_node->forward = next;
                    next->backward = new_node;
                    new_node->backward->forward = new_node;
                    head = new_node;

                    struct client_info *info = malloc(sizeof (struct client_info));
                    info->ipc_id = client_id;

                    head->data = info;

                    printf(GRN "New client: %ld\n" RESET, client_id);
                } else if (type == IPC_DISCONNECT) {
                    // Client disconnected, remove from linked list

                    struct ll_node* node = find_client(root, client_id);

                    if (node == NULL) {
                        printf(RED "Could not find client %ld to disconnect\n" RESET, client_id);
                        continue;
                    }

                    struct client_info* client_info = (struct client_info*) node->data;

                    if (node == head) {
                        head = node->backward;
                    }

                    node->backward->forward = node->forward;
                    node->forward->backward = node->backward;

                    if (selected_session == client_info) {
                        selected_session = NULL;
                    }

                    free(client_info);
                    free(node);

                    printf(YEL "Client %ld disconnected\n" RESET, client_id);

                } else if (type == IPC_CLIENT_INFO) {
                    struct client_info* client_info_msg = &message->client_info_message;

                    if (client_info_msg == NULL) {
                        printf(RED "Client info is null!\n" RESET);
                        continue;
                    }

                    // Find client in Linked List
                    struct ll_node* node = find_client(root, client_id);

                    if (node == NULL) {
                        printf(RED "Could not find client %ld\n" RESET, client_id);
                        continue;
                    }

                    // Update client info

                    struct client_info* current_info = (struct client_info*) node->data;

                    memcpy(current_info, client_info_msg, sizeof(struct client_info));

                    printf(GRN "Updated client info for %ld\n" RESET, client_id);
                } else {
                    printf(RED "Unknown message type %d\n" RESET, (int)type);
                }
            }
        }


        // Parse input
        parse_and_call(i_map, input, root, &selected_session);
    }
}