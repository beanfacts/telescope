/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope JSON Server Demo
    Copyright (c) 2021 Tim Dettmar
*/

#include "libtdma.h"
#include "json_srv.h"

#include <sys/socket.h>

int main(int argc, char **argv)
{

    // Process command line arguments.
    static struct option long_options[] =
    {
        {"host",        required_argument,  0,  'h'},
        {"nc_port",     required_argument,  0,  'n'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    struct in_addr host;
    uint8_t ok_flag = 0;
    uint16_t port;

    while (1)
    {
        int c = getopt_long_only(argc, argv, "h:n:", long_options, &option_index);
        printf("[%s]\n", optarg);

        if (c == -1)
            break;

        switch (c)
        {
                
            case 'h':
                if (!inet_aton(optarg, &host))
                {
                    fprintf(stderr, "Could not convert IP address...\n");
                    break;
                }
                ok_flag = ok_flag | 1;
                break;
            case 'n':
                port = (uint16_t) atoi(optarg);
                ok_flag = ok_flag | 2;
                break;
        }
    }

    if (ok_flag != 3)
    {
        fprintf(stderr, "Invalid argument count\n");
    }
    
    // Create and bind the data channel socket
    struct sockaddr_in servaddr;
    int sockfd = td_create_server(host.s_addr, port, SOCK_STREAM, &servaddr);
    if (sockfd < 0)
    {
        fprintf(stderr, "Could not create the socket.\n");
        return 1;
    }
    else
    {
        printf("Server listening on %s:%d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    }
    
    // Allocate memory to handle a new client
    int sa_len = sizeof(struct sockaddr);
    ClientInfo *client_info = malloc(sizeof(ClientInfo));

    client_info->client_fd = accept(sockfd, &client_info->client_data, &sa_len); 
    if (client_info->client_fd < 0)
    { 
        printf("Server could not accept the client.\n");
        free(client_info);
        return 1;
    }

    void *buf = malloc(65536);
    int frame_size = 4;
    int bytes_read = 0;

    fprintf(stdout, 
        "Client info: %s:%d\n", 
        inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

    while (bytes_read < frame_size)
    {
        int res = recv(client_info->client_fd, buf, frame_size - bytes_read, 0);
        if (res != -1)
        {
            bytes_read += res;
        }
    }

    if (ntohl(*(uint32_t *) buf) == THDR)
    {
        printf("Hi\n");
    }
    else
    {
        printf("Invaild magic number\n");
    }
}