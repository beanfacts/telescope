/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (RDMA_CM)
    Copyright (c) 2021 Telescope Project
*/

#include <cstdio>
#include <stdexcept>
#include "transport_rdmacm.hpp"

/* ------------------ Common Transport Functions ------------------ */

/* ------------------ Server Transport Functions ------------------ */

void tsc_server_rdma::init_server(const char *host, const char *port,
        struct rdma_addrinfo *hints, struct rdma_addrinfo **host_res, 
        struct ibv_qp_init_attr *init_attr, int backlog )
{
    int ret;

    // Default values for optional attributes

    if (!hints)
    {
        struct rdma_addrinfo _hints;
        memset(&_hints, 0, sizeof(_hints));
        _hints.ai_port_space    = RDMA_PS_TCP;
        _hints.ai_flags         = RAI_PASSIVE;
        hints = &_hints;
    }

    if (!host_res)
    {
        struct rdma_addrinfo *_host_res;
        host_res = &_host_res;
    }

    if (!init_attr)
    {
        struct ibv_qp_init_attr _init_attr;
        _init_attr.cap.max_send_wr      = 10;
        _init_attr.cap.max_recv_wr      = 10;
        _init_attr.cap.max_send_sge     = 10;
        _init_attr.cap.max_recv_sge     = 10;
        _init_attr.cap.max_inline_data  = 128;
        _init_attr.sq_sig_all           = 1;
        _init_attr.qp_type              = IBV_QPT_RC;
        init_attr = &_init_attr;
    }

    // Resolve host/port into rdma_addrinfo struct
    printf("Attempting to listen on %s:%s\n", host, port);
    ret = rdma_getaddrinfo(host, port, hints, host_res);
    if (ret)
        throw std::runtime_error("Listen failed.");
    
    // Create RDMA endpoint based on user information
    ret = rdma_create_ep(&(tsc_server_rdma::connid), *host_res, NULL, init_attr);
    if (ret)
        throw std::runtime_error("Could not create connection\n");

    printf("Created RDMA connection\n");

    // Start listening for incoming connections
    ret = rdma_listen(tsc_server_rdma::connid, backlog);
    if (ret)
        throw std::runtime_error("Failed to listen on RDMA channel.\n");

    printf("Listening on RDMA channel. (%p)\n", (void *) tsc_server_rdma::connid);
}

void tsc_server_rdma::init_server(const char *host, const char *port, int backlog)
{
    tsc_server_rdma::init_server(host, port, NULL, NULL, NULL, 1);
}

/* ------------------ Client Transport Functions ------------------ */