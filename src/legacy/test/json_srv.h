#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

typedef struct {
    struct sockaddr client_data;
    int client_fd;
} ClientInfo;