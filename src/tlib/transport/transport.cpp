/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library
    Copyright (c) 2021 Telescope Project
*/

// getaddrinfo
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <netdb.h>

#include "transport.hpp"
#include "transport_rdmacm.hpp"


int tsc_set_sockaddr(const char *address_str, const char *port, int is_server, struct sockaddr_in *out_addr)
{
    int ret;
    in_addr raw_addr;

    ret = inet_aton(address_str, &raw_addr);
    if (!ret)
    {
        fprintf(stderr, "Could not resolve address.\n");
        return 1;
    }

    char *endptr;
    errno = 0;
    long _port = strtol(port, &endptr, 10);
    if (_port > 65535 || _port < 1)
    {
        fprintf(stderr, "Port %s invalid.\n", port);
        return 1;
    }
    else if (errno)
    {
        fprintf(stderr, "Port %s invalid: %s\n", port, strerror(errno));
        return 1;
    }
    
    memset(out_addr, 0, sizeof(*out_addr));
    out_addr->sin_family    = AF_INET;
    out_addr->sin_addr      = raw_addr;
    out_addr->sin_port      = htons(_port);

    return 0;
}


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

    ret = bind(sock, (struct sockaddr *) sa, sizeof(*sa));
    if (ret)
    {
        fprintf(stderr, "Bind failed: %s\n", strerror(errno));
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


struct tsc_server *tsc_alloc_server()
{
    
    // Allocate space to store all server metadata
    tsc_server *out_server  = (tsc_server *)        calloc(1, sizeof(tsc_server));
    out_server->tcp_server  = (tsc_server_tcp *)    calloc(1, sizeof(tsc_server_tcp));
    out_server->init_server = (tsc_server_init *)   calloc(1, sizeof(tsc_server_init));
    out_server->rdma_server = (tsc_server_rdmacm *) calloc(1, sizeof(tsc_server_rdmacm));
    out_server->ucx_server  = (tsc_server_ucx *)    calloc(1, sizeof(tsc_server_ucx));

    // Check if any allocations failed
    if (    out_server && out_server->tcp_server && out_server->init_server 
            && out_server->rdma_server && out_server->ucx_server )
    {
        return out_server;
    }

    // Free and return error if failed
    if (out_server)
    {
        if (out_server->tcp_server)     { free(out_server->tcp_server); }
        if (out_server->init_server)    { free(out_server->init_server); }
        if (out_server->rdma_server)    { free(out_server->rdma_server); }
        if (out_server->ucx_server)     { free(out_server->ucx_server); }
        free(out_server);
    }

    errno = ENOMEM;
    return NULL;

}


int tsc_free_server(struct tsc_server *server)
{
    return 1;
}


struct tsc_server *tsc_start_servers(struct tsc_transport *init_transport,
        struct tsc_transport *transports, int max_clients)
{
    
    int ret;
    int exit_flag = 0;
    tsc_server *out_server = tsc_alloc_server();
    
    struct sockaddr_in server_addr;

    printf("Attempting to start listener on: %s:%s\n", init_transport->host, init_transport->port);

    // Convert host string/port into sockaddr struct.
    ret = tsc_set_sockaddr(init_transport->host, init_transport->port, 1, &server_addr);
    if (ret)
    {
        fprintf(stderr, "Cannot configure sockaddr struct\n");
        return NULL;
    }

    printf("Config: %s, %d, %d\n", inet_ntoa(server_addr.sin_addr), server_addr.sin_family, ntohs(server_addr.sin_port));

    // Bind and listen on specified port
    out_server->init_server->sockfd = tsc_init_tcp_server(&server_addr, max_clients);
    if (out_server->init_server->sockfd < 0)
    {
        fprintf(stderr, "Failed to create socket.\n");
        return NULL;
    }
    
    // Initialise all the transports the user selected
    for (int i = 0;;i++)
    {
        switch (transports[i].flag)
        {
            case TSC_TRANSPORT_NONE:
            {
                exit_flag = 1;
                break;
            }
            case TSC_TRANSPORT_TCP:
            {
                struct sockaddr_in tcp_sockaddr;
                memset(&tcp_sockaddr, 0, sizeof(tcp_sockaddr));
                ret = tsc_set_sockaddr(transports[i].host, transports[i].port, 1, &tcp_sockaddr);
                if (ret)
                {
                    fprintf(stderr, "Cannot initialise TCP server.\n");
                    goto out_error;
                }
                out_server->tcp_server->sockfd = tsc_init_tcp_server(&tcp_sockaddr, max_clients);
                if (out_server->tcp_server->sockfd < 0)
                {
                    fprintf(stderr, "Failed to create socket.\n");
                    goto out_error;
                }
                break;
            }
            case TSC_TRANSPORT_RDMACM:
            {
                out_server->rdma_server->listen_id = tsc_init_rdma_server(
                    transports[i].host, transports[i].port, NULL, NULL, NULL );
                if (!out_server->rdma_server->listen_id)
                {
                    fprintf(stderr, "Failed to create RDMA server.\n");
                    goto out_error;
                }
                break;
            }
            case TSC_TRANSPORT_UCX:
            {
                fprintf(stderr, "UCX transport not implemented.\n");
                goto out_error;
            }
        }
        if (exit_flag) { break; }
    }

    return out_server;

out_error:
    tsc_free_server(out_server);
    return NULL;

}


struct tsc_client *tsc_wait_client(struct tsc_server *server)
{
    int fd;
    sockaddr sa;
    socklen_t sa_len;
    fd = accept(server->init_server->sockfd, &sa, &sa_len);
    if (fd < 0)
    {
        fprintf(stderr, "Could not accept client.\n");
        return NULL;
    }

    struct tsc_client *client = 
        (struct tsc_client *) calloc(1, sizeof(struct tsc_client));


}