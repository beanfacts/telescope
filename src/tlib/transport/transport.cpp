/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library
*/

// getaddrinfo
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#include <sys/socket.h>
#include <transport.hpp>

/*
    Initialise a socket server.
    sa:         Sockaddr struct.
    backlog:    Max pending connections.
    Returns:    fd, or -1 on errors.
*/
int tsc_init_tcp_server(struct sockaddr_in *sa, int backlog)
{
    // Initialise the channel
    int ret;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "Could not create sockfd.\n");
        return -1;
    }

    // Enable IPv6-mapped IPv4 connections
    int flag = 0;
    ret = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &flag, sizeof(flag));
    if (ret < 0)
    {
        fprintf(stderr, "Could not modify socket parameters.\n");
        return -1;
    }

    ret = bind(sock, (sockaddr *) sa, sizeof(sa));
    if (ret)
    {
        fprintf(stderr, "Bind failed.\n");
        return -1;
    }

    ret = listen(sock, backlog);
    if (ret < 0)
    {
        fprintf(stderr, "Listen failed.\n");
        return -1;
    }

    return sock;
}

/*
    Allocate resources for a server.
    init_transport: Control channel parameters.
    transports:     List of transport types to use.
    max_clients:    Maximum simultaneous clients.
*/
struct tsc_server *tsc_start_servers(struct tsc_transport *init_transport,
        struct tsc_transport *transports, int max_clients)
{
    
    #pragma region init_control_channel
    
    int ret;
    tsc_server *out_server = (tsc_server *) malloc(sizeof(tsc_server));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    ret = inet_aton(init_transport->host, &server_addr.sin_addr);
    if (ret)
    {
        fprintf(stderr, "Could not resolve address.\n");
        return NULL;
    }

    char *endptr;
    errno = 0;
    long _port = strtol(init_transport->port, &endptr, 10);
    if (_port > 65535 || _port < 1)
    {
        fprintf(stderr, "Port %s invalid.\n", init_transport->port);
        return NULL;
    }
    else if (errno)
    {
        fprintf(stderr, "Port %s invalid: %s\n", init_transport->port, strerror(errno));
        return NULL;
    }

    out_server->sockfd = tsc_init_tcp_server(&server_addr, max_clients);
    if (out_server->sockfd < 0)
    {
        fprintf(stderr, "Failed to create socket.\n");
        return NULL;
    }

    #pragma endregion
    #pragma region init_transports
    
    // 
    int exit_flag = 0;
    while (1)
    {
        switch (transports->flag)
        {
            case 0:
                exit_flag = 1;
                break;
            case TSC_TRANSPORT_TCP:

        }

        if (exit_flag) { break; }

    }

    #pragma endregion
}