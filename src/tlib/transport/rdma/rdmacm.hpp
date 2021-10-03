/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (RDMA_CM)
    Copyright (c) 2021 Telescope Project
*/

#pragma once

#include "transport/transport.hpp"
#include "transport/base.hpp"
#include "transport/rdma/rdmacm_extras.h"

#include <list>
#include <vector>
#include <rdma/rdma_verbs.h>
#include <rdma/rdma_cma.h>


/* ------------------ Common Transport Functions ------------------ */

namespace tsc_util_rdmacm {

    tsc_rdma_id_h connect(const char *host, const char *port, int type);

};

/* ------------------ Client Transport Functions ------------------ */

class tsc_communicator_nb_rdma : public tsc_communicator_nb {

public:

    /*  Init function specific for RDMA_CM comms */
    void init(tsc_rdma_id_h id);
    
    /*  From tsc_communicator_nb */
    int get_transport_type() = 0;
    tsc_mr_h register_mem(void *buf, size_t length, int flags) = 0;
    void deregister_mem(tsc_mr_h mr) = 0;
    uint64_t send_msg(void *buf, size_t length) = 0;
    uint64_t send_msg(tsc_mr_h mr, void *buf, size_t len) = 0;
    uint64_t recv_msg(tsc_mr_h mr, void *buf, size_t len) = 0;
    uint64_t write_msg(tsc_mr_h local_mr, tsc_mr_h remote_mr,
            void *local_addr, void *remote_addr, size_t length) = 0;
    uint64_t read_msg(tsc_mr_h local_mr, tsc_mr_h remote_mr,
            void *local_addr, void *remote_addr, size_t length);
    int poll_sq(uint64_t *out, bool block) = 0;
    int poll_rq(uint64_t *out, bool block) = 0;
    
    struct rdma_cm_id       *connid;

private:

    struct _tsc_send_ctx {
        void    *alloc_internal;    // Pointer to internal malloc region
        ibv_mr  *tmp_mr;            // Pointer to dynamic MR
    };

    struct _tsc_recv_ctx {
        bool    a;                  // No need for anything on receive side rn
    };

    struct ibv_mr           *snd_mr;
    struct ibv_mr           *rcv_mr;
    tsc_malloc              *snd_mgr = 0;
    tsc_malloc              *rcv_mgr = 0;

    size_t                  max_inline = 0;

    struct ibv_qp_attr      qp_attr = {};
    struct ibv_qp_init_attr qp_init_attr = {};

    std::unordered_map<uint64_t, _tsc_send_ctx> send_set;
    std::unordered_map<uint64_t, _tsc_recv_ctx> recv_set;
};