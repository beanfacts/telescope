/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library
    Copyright (c) 2021 Telescope Project
*/

#ifndef T_TRANSPORT_H
#define T_TRANSPORT_H

#include <iostream>
#include <vector>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <ucp/api/ucp.h>

#include "transport_rdmacm.hpp"
#include "transport_tcp.hpp"
#include "transport_base.hpp"

/* ------------------ Common Transport Functions ------------------ */

/* ------------------ Server Transport Functions ------------------ */

/* Server-side representation of a client. */
class tsc_client_repr {

private:

    tsc_transport_type              transport_type;
    std::vector<struct ibv_mr *>    client_mrs;
    
    struct rdma_cm_id   *cm_id  =  0;
    int                 init_fd = -1;
    int                 conn_fd = -1;

};

/* Struct to hold supported classes used by the Telescope server */
struct tsc_server_data {
    tsc_server_tcp              meta_server;    // TCP server   (initialization / data exchange phase)
    tsc_server_tcp              tcp_server;     // TCP server   (screen capture phase)
    tsc_server_rdma             rdma_server;    // RDMA server  (screen capture phase)          
};

/*  Server class.
    Stores all the active transport types as well as
    all the client representations in a single object. */
class tsc_server : private tsc_server_base {

public:
    
    void init_servers(struct tsc_transport *init_transport,
        struct tsc_transport *transports, int max_clients);

    tsc_client_repr get_client();

private:

    struct tsc_server_data server;
    std::vector<tsc_client_repr> clients;

};

/* ------------------ Client Transport Functions ------------------ */

struct tsc_client_data {
    tsc_client_tcp              meta_server;
    tsc_client_tcp              tcp_server;
    tsc_client_rdma             rdma_server;
}

/*  Client class.
    Stores all the active transport types as well as
    all the client representations in a single object. */
class tsc_client : private tsc_client_base {

public:

    void init_client(struct tsc_transport *transport);

    void connect();

private:

    struct tsc_client_data client;
}

#endif