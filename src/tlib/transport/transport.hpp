/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library
*/

#include <iostream>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <ucp/api/ucp.h>

#include "transport_common.h"

enum tsc_transport_type {
    TSC_TRANSPORT_NONE      = 0,        // Dummy server
    TSC_TRANSPORT_TCP    = 1,        // TCP
    TSC_TRANSPORT_UCX       = 2,        // Unified Communication X (UCX)
    TSC_TRANSPORT_RDMACM    = 3         // RDMA_CM + InfiniBand Verbs
};



/*
    Server-side representation of a client.
    Supports all transport types.
*/
struct tsc_Client {
    enum tsc_transport_type     cli_transport_type;
    enum tsc_capture_type       cli_cap_type;
    int                         mgmt_connfd;
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
    int                         sockfd; // Metadata exchange file descriptor
};

/*
    RDMA_CM Server.
*/
struct tsc_server_rdmacm {
    struct rdma_cm_id   *listen_id;     // RDMA_CM connection identifier
    struct rdma_cm_id   **endpoints;    // Connection ID pointer list
    int                 active_conns;   // Number of active client connections   
};

/*
    Fallback TCP transmission system.
*/
struct tsc_server_tcp {
    int     sockfd;
    int     *endpoints;
    int     active_conns;
};

/*
    TCP server for client information exchange.
*/
struct tsc_server_init {
    int     sockfd;
    int     *endpoints;
    int     active_conns;
};

/*
    UCX Server.
*/
struct tsc_server_ucx {
    ucp_ep_h    *endpoints;
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

struct tsc_server *tsc_start_servers(
        struct tsc_transport *init_transport,
        struct tsc_transport *transports, int max_clients);