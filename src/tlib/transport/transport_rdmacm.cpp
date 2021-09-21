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
        struct ibv_qp_init_attr *init_attr,
        int backlog )
{
    int ret;

    struct rdma_addrinfo _hints;
    struct ibv_qp_init_attr _init_attr;
    struct rdma_addrinfo *_host_res = nullptr;

    bool keep_info = true;

    // Default values for optional attributes

    if (!hints)
    {
        std::cout << "defaults1\n";
        memset(&_hints, 0, sizeof(_hints));
        _hints.ai_port_space    = RDMA_PS_TCP;
        _hints.ai_flags         = RAI_PASSIVE;
        hints = &_hints;
    }
    
    if (!host_res)
    {
        host_res = &_host_res;
        keep_info = false;
    }

    if (!init_attr)
    {
        std::cout << "defaults2\n";
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
        throw std::runtime_error("rdma_getaddrinfo() failed.");
    
    // Create RDMA endpoint based on user information
    ret = rdma_create_ep(&connid, *host_res, NULL, init_attr);
    if (ret)
        throw std::runtime_error("Could not create connection\n");

    printf("Created RDMA connection\n");

    // Start listening for incoming connections
    ret = rdma_listen(connid, backlog);
    if (ret)
        throw std::runtime_error("Failed to listen on RDMA channel.\n");

    printf("Listening on RDMA channel. (%p)\n", (void *) connid);
  
    if (!keep_info)
    {
        rdma_freeaddrinfo(*host_res);
    }

}

void tsc_server_rdma::init_server(const char *host, const char *port, int backlog)
{
    tsc_server_rdma::init_server(host, port, NULL, NULL, NULL, backlog);
}

rdma_cm_id *tsc_server_rdma::accept()
{
    int ret;

    struct rdma_cm_id *new_id;
    std::cout << "Waiting for request on channel " << connid << "\n";
    ret = rdma_get_request(connid, &new_id);
    if (ret)
    {
        throw std::runtime_error("Failed to read connection request.");
    }

    std::cout << "New conection: " << new_id << "\n";
    ret = rdma_accept(new_id, NULL);
    if (ret)
    {
        rdma_destroy_ep(new_id);
        throw std::runtime_error("Could not accept connection.");
    }

    return new_id;
}


/* ------------------ Client Transport Functions ------------------ */

void tsc_client_rdma::connect(const char *host, const char *port)
{
    int ret;
    struct rdma_addrinfo hints = {};
    struct rdma_addrinfo *host_res;
    ret = rdma_getaddrinfo(host, port, &hints, &host_res);
    if (ret)
    {
        throw std::runtime_error("Could not resolve host.");
    }

    //connid = (rdma_cm_id *) malloc(sizeof(struct rdma_cm_id));
    struct ibv_qp_init_attr init_attr = {};

    init_attr.cap.max_send_wr = 10;
    init_attr.cap.max_recv_wr = 10;
    init_attr.cap.max_send_sge = 10;
    init_attr.cap.max_recv_sge = 10;
    init_attr.qp_type = IBV_QPT_RC;
    init_attr.qp_context = connid;
    init_attr.sq_sig_all = 1;

    ret = rdma_create_ep(&connid, host_res, NULL, &init_attr);
    if (ret)
    {
        throw std::runtime_error("RDMA endpoint creation failed.");
    }

    ret = rdma_connect(connid, NULL);
    if (ret)
    {
        throw std::runtime_error("Server connection failed.");
    }
}