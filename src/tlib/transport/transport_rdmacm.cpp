/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (RDMA_CM)
    Copyright (c) 2021 Telescope Project
*/

#include <stdio.h>
#include "transport_rdmacm.hpp"

struct rdma_cm_id *tsc_init_rdma_server(const char *host, const char *port,
        struct rdma_addrinfo *hints, struct rdma_addrinfo **host_res, 
        struct ibv_qp_init_attr *init_attr )
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
    {
        fprintf(stderr, "Could not resolve host.\n");
        return NULL;
    }
    
    // Create RDMA endpoint based on user information
    struct rdma_cm_id *cm_id;
    ret = rdma_create_ep(&cm_id, *host_res, NULL, init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection\n");
        return NULL;
    }
    printf("Created RDMA connection\n");

    // Start listening for incoming connections
    ret = rdma_listen(cm_id, 0);
    if (ret)
    {
        fprintf(stderr, "Failed to listen on RDMA channel.\n");
        return NULL;
    }

    printf("Listening on RDMA channel. (%p)\n", (void *) cm_id);
    return cm_id;
}