/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (RDMA_CM)
    Copyright (c) 2021 Telescope Project
*/

#ifndef T_TRANSPORT_RDMACM_H
#define T_TRANSPORT_RDMACM_H

#include <rdma/rdma_verbs.h>
#include <rdma/rdma_cma.h>

struct rdma_cm_id *tsc_init_rdma_server(const char *host, const char *port,
        struct rdma_addrinfo *hints, struct rdma_addrinfo **host_res, 
        struct ibv_qp_init_attr *init_attr );

#endif