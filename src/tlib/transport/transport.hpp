/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library
    Copyright (c) 2021 Telescope Project
*/

#ifndef T_TRANSPORT_H
#define T_TRANSPORT_H

#include <iostream>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <ucp/api/ucp.h>

typedef enum {
    TSC_TRANSPORT_NONE      = 1,        // Dummy server
    TSC_TRANSPORT_TCP       = 1 << 1,   // TCP
    TSC_TRANSPORT_UCX       = 1 << 2,   // Unified Communication X (UCX)
    TSC_TRANSPORT_RDMACM    = 1 << 3    // RDMA_CM + InfiniBand Verbs
} tsc_transport_type;


/*
    Server-side representation of a client.
    Supports all transport types.
*/
struct tsc_client {
    tsc_transport_type          transport_type;
    tsc_capture_type            capture_type;
    int                         init_connfd;
    union {
        struct {
            struct rdma_cm_id   *cm_connid;
            struct ibv_mr       *cm_client_mr;
        };
        struct {
            ucp_ep_h            ucp_endpoint;
            ucp_mem_h           ucp_client_mr;
            ucp_rkey_h          ucp_client_rkey;
        };
        struct {
            int                 tcp_connfd;
        };
    };
};

struct tsc_server {
    struct tsc_server_init      *init_server;    // TCP server   (initialization / data exchange phase)
    struct tsc_server_tcp       *tcp_server;     // TCP server   (screen capture phase)
    struct tsc_server_rdmacm    *rdma_server;    // RDMA server  (screen capture phase)
    struct tsc_server_ucx       *ucx_server;     // UCX server   (screen capture phase)
};

/*
    RDMA_CM Server.
*/
struct tsc_server_rdmacm {
    struct rdma_cm_id   *listen_id;     // RDMA_CM connection identifier
    int                 active_conns;   // Number of active client connections   
};

/*
    Fallback TCP transmission system.
*/
struct tsc_server_tcp {
    int     sockfd;                     // TCP socket fd
    int     active_conns;
};

/*
    TCP server for client information exchange.
*/
struct tsc_server_init {
    int     sockfd;
    int     active_conns;
};

/*
    UCX Server.
*/
struct tsc_server_ucx {
    ucp_ep_h    *listen_ep;
    int         active_conns;
};

#define TSC_SERVER_USE_TCP      1 << 1
#define TSC_SERVER_USE_UCX      1 << 2
#define TSC_SERVER_USE_RDMACM   1 << 3

struct tsc_transport {
    int         flag;
    char        *host;
    char        *port;
    void        *attr;
};

/*
    Populate a sockaddr struct from string input.
    address_str:    Address string.
    port:           Port string.
    is_server:      Server or client?
    addr:           Output address.
*/
int tsc_set_sockaddr(const char *address_str, const char *port, int is_server,
        struct sockaddr_in *out_addr);

/*
    Allocate network resources for a server.
    init_transport: Control channel parameters.
    transports:     List of transport types to use.
    max_clients:    Maximum simultaneous clients.
    Returns:        Server struct populated with requested transports.
*/
struct tsc_server *tsc_start_servers(struct tsc_transport *init_transport,
        struct tsc_transport *transports, int max_clients );

/*
    Free allocated server struct space, closing all connections if
    any were opened and referenced inside this struct.
    server:     Input server.
    Returns:    0 if successful, 1 on error.
*/
int tsc_free_server(struct tsc_server *server);

/*
    Allocate server struct space.
    Returns:    Server struct with memory allocated in all fields.
*/
struct tsc_server *tsc_alloc_server();

/*
    Initialise an RDMA server using RDMA_CM.
    host:       Host string (in)
    port:       Port string (in)
    hints:      Connection hints (in, optional)
    host_res:   Connection hints used (out, optional)
    init_attr:  RDMA_CM QP init attributes (in, optional)
    Returns:    RDMA_CM connection id
*/
struct rdma_cm_id *tsc_init_rdma_server(const char *host, const char *port,
        struct rdma_addrinfo *hints, struct rdma_addrinfo **host_res, 
        struct ibv_qp_init_attr *init_attr );


/*
    Initialise a socket server.
    sa:         Sockaddr struct.
    backlog:    Max pending connections.
    Returns:    fd, or -1 on errors.
*/
int tsc_init_tcp_server(struct sockaddr_in *sa, int backlog);

/*
    Wait for a client to connect to the init channel.
    server:     Server data struct.
    Returns:    Connection FD.
*/
struct tsc_client *tsc_wait_client(struct tsc_server *server);


#endif