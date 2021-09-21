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

#include "transport_rdmacm.hpp"
#include "transport_tcp.hpp"
#include "transport_base.hpp"

/* ------------------ Common Transport Functions ------------------ */

/* ------------------ Server Transport Functions ------------------ */


#define TSC_MT_CLIENT_HELLO         1
#define TSC_MT_CLIENT_HELLO_REPLY   1 << 1

typedef struct {
    int     size;
    char    *buf;
} tsc_mbuf;


/* Receive delimited message. */
tsc_mbuf tsc_recv_msg(int fd);

/* Send delimited message. */
void tsc_send_msg(char *buf, int fd);

/* Server-side representation of a client. */
class tsc_client_repr {

public:

    /* Attach the file descriptor of the client's control channel */
    void attach_init_fd(int __fd)
    {
        init_fd = __fd;
    }

    /* Attach RDMACM ID of the client. */
    void attach_cm_id(struct rdma_cm_id *__cm_id)
    {
        cm_id = __cm_id;
        transport_type = TSC_TRANSPORT_RDMACM;
    }

    /* Get client control channel fd. */
    int get_init_fd()
    {
        return init_fd;
    }

    /* Get client RDMACM ID. */
    rdma_cm_id *get_cm_id()
    {
        return cm_id;
    }

    tsc_mbuf recv_msg()
    {
        return tsc_recv_msg(init_fd);
    }

    void send_msg(char *buf)
    {
        tsc_send_msg(buf, init_fd);
    }

    /* Advance to the next frame. */
    void advance_frame();

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
class tsc_server : public tsc_server_base {

public:
    
    void init_servers(struct tsc_transport *init_transport,
        struct tsc_transport *transports, int max_clients);


    /* Wait for a client on the control channel. */
    tsc_client_repr get_client();

    /* Wait for a client on one of the data channels. */
    void add_client_channel(tsc_client_repr *client, int transport_flag);

private:

    struct tsc_server_data server;
    std::vector<tsc_client_repr> clients;

};

/* ------------------ Client Transport Functions ------------------ */

struct tsc_client_data {
    tsc_client_tcp              meta_server;
    tsc_client_tcp              tcp_server;
    tsc_client_rdma             rdma_server;
};

/*  Client class.
    Stores all the active transport types as well as
    all the client representations in a single object. */
class tsc_client : public tsc_client_base {

public:

    void init_transport(struct tsc_transport *transport);

    void init_control_channel(const char *host, const char *port);

    tsc_mbuf recv_msg();

    void send_msg(char *buf);

private:

    struct tsc_client_data client;

};

#endif