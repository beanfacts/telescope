/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (RDMA_CM)
    Copyright (c) 2021 Telescope Project
*/

#pragma once

#include "transport_base.hpp"

#include <rdma/rdma_verbs.h>
#include <rdma/rdma_cma.h>

/* ------------------ Common Transport Functions ------------------ */

/* ------------------ Server Transport Functions ------------------ */

class tsc_server_rdma : public tsc_server_base {

public:

    void init_server(const char *host, const char *port,
            struct rdma_addrinfo *hints, struct rdma_addrinfo **host_res,
            struct ibv_qp_init_attr *init_attr, int backlog );

    void init_server(const char *host, const char *port, int backlog);

private:

    struct rdma_cm_id *connid;

};

/* ------------------ Client Transport Functions ------------------ */

class tsc_client_rdma : public tsc_client_base {

public:

    void connect(const char *host, const char *port);

};