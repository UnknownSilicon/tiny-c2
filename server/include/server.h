#include <arpa/inet.h>
#include "ipc.h"

void handle_sigchld(int signum);

void start_server(in_addr_t* host, short port, struct message_queues* i_map);