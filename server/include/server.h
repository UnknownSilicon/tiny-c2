#include <arpa/inet.h>

void handle_sigchld(int signum);

void start_server(in_addr_t* host, short port);