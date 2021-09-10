/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope JSON Client Demo
    Copyright (c) 2021 Tim Dettmar
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "libtdma.h"

typedef struct {
    uint64_t        fmem_limit;
    uint64_t        pmem_limit;
    struct in_addr  host;
    uint16_t        rdma_port;
    uint16_t        nc_port;
    uint16_t        pages;

} ClientOpts;

// ./json_cli --host <host or ip> --port <port no>
int main(int argc, char **argv)
{
    static struct option long_options[] =
    {
        {"host",        required_argument,  0,  'h'},
        {"nc_port",     required_argument,  0,  'n'},
        {"fmem_limit",  required_argument,  0,  'f'},
        {"pmem_limit",  required_argument,  0,  'l'},
        {"pages",       required_argument,  0,  'p'},
        {0, 0, 0, 0}
    };

    ClientOpts client_opts;
    memset(&client_opts, 0, sizeof(ClientOpts));
    
    int c;
    int option_index = 0;
    
    // Default values for Telescope Client
    client_opts.fmem_limit = 32 * 1048576;      // 32 MB page limit
    client_opts.pmem_limit = 128 * 1048576;     // 128 MB memory limit
    client_opts.pages = 2;                      // 2 pages

    uint8_t ok_flag = 0;
    
    while (1)
    {
        c = getopt_long_only(argc, argv, "h:n:f::l::p::", long_options, &option_index);
        
        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                if (!inet_aton(optarg, &client_opts.host))
                    fprintf(stderr, "Could not convert IP address...\n");
                    ok_flag = ok_flag | 1;
                    break;
            case 'n':
                client_opts.nc_port = (uint16_t) atoi(optarg);
                ok_flag = ok_flag | 2;
                break;
            case 'f':
                client_opts.fmem_limit = strtoul(optarg, NULL, 10) * 1024UL;
                break;
            case 'l':
                client_opts.pmem_limit = strtoul(optarg, NULL, 10) * 1024UL;
                break;
            case 'p':
                client_opts.pages = (uint16_t) atoi(optarg);
                break;
        }
    }

    if (ok_flag != 3)
    {
        printf(
            "Required arguments\n\n"
            "   --host <host>       Server IP address\n"
            "   --nc_port <port>    TCP negotiation channel port\n\n"
            "Optional Arguments\n\n"
            "   --fmem_max <n>      Maximum memory per frame in KB\n"
            "   --pmem_max <n>      Pinned memory limit in KB\n"
            "   --pages <n>         Number of logical memory regions to use for frame buffering\n\n"
        ,   argv[0]);
        return 1;
    }
    
    printf( "-- Memory Information --\n"
            "Page memory limit: %dK, Pinned memory limit: %dK\n"
            "Pages: %d -> Max memory usage: %dK\n"
            "-- Server Information --\n"
            "Host: %s (RDMA Port: %d; TCP Port: %d)\n",
            client_opts.fmem_limit / 1024, client_opts.pmem_limit / 1024,
            client_opts.pages, (client_opts.fmem_limit * client_opts.pages) / 1024,
            inet_ntoa(client_opts.host), client_opts.rdma_port, client_opts.nc_port);
    
    struct sockaddr_in servaddr;
    int sockfd = td_create_server(client_opts.host.s_addr, client_opts.nc_port, SOCK_STREAM, &servaddr);
    

}