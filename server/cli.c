#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cli.h"
#include "ipc.h"
#include "util.h"

int str_eq(char* part, const char* literal) {
    size_t len = strlen(literal);

    return strncmp(part, literal, len) == 0;
}

void parse_and_call(char* input) {
    char* saveptr;
    char* tok = strtok_r(input, " \n", &saveptr);

    if (tok == NULL) {
        return;
    }

    if (str_eq(tok, "exit")) {
        if (kill(getppid(), SIGTERM) != 0) {
            kill(getppid(), SIGKILL);
        }

        sleep(5);
        printf("Couldn't exit after 5 seconds. Sending SIGKILL...");
        kill(getppid(), SIGKILL);
        exit(0);
    }
}

void start_cli(struct init_map* i_map) {
    // Store a linked list of connections
    struct ll_node* root = malloc(sizeof (struct ll_node));
    root->backward = root;
    root->forward = root;
    root->data = NULL;

    struct ll_node* head = root;

    char* curr_sess = "";
    char input[100];

    while (1) {
        sem_wait(&i_map->sem);
        int num = i_map->curr_conn;
        // Could release semaphore here for a bit as we create nodes, but it's cleaner to just keep it
        for (int i=0; i<num; i++) {
            struct connection* conn = i_map->conns[i];
            i_map->conns[i] = NULL;

            // Add connection to list
            struct ll_node* new_node = malloc(sizeof (struct ll_node));
            new_node->backward = head;
            struct ll_node* next = head->forward;
            new_node->forward = next;
            next->backward = new_node;
            new_node->backward->forward = new_node;
            head = new_node;

            head->data = conn;
        }

        i_map->curr_conn = 0;
        sem_post(&i_map->sem);

        printf("%s>> ", curr_sess);
        if (fgets(input, 100, stdin) == NULL) {
            printf("Error reading input.\n");
            continue;
        }

        // Parse input
        parse_and_call(input);
    }
}