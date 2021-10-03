/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (RDMA_CM)
    Copyright (c) 2021 Telescope Project
*/

#include <cstdio>
#include <stdexcept>

#include "transport/rdma/rdmacm.hpp"

/* ------------------ Common Transport Functions ------------------ */

void tsc_communicator_nb_rdma::init(tsc_rdma_id_h id)
{
    int ret;
    connid = (rdma_cm_id *) id.addr;
    
    /*  Allocate send and receive buffers. */
    snd_mgr = new tsc_malloc(1 << 16, 4096);
    rcv_mgr = new tsc_malloc(1 << 16, 4096);
    
    /*  Register allocated buffers for RDMA operations. */
    snd_mr = rdma_reg_msgs(connid, snd_mgr->get_addr(), 4096);
    if (!snd_mr)
        throw std::runtime_error("Send MR registration failed.");

    rcv_mr = rdma_reg_msgs(connid, rcv_mgr->get_addr(), 4096);
    if (!rcv_mr)
        throw std::runtime_error("Recv MR registration failed.");

    /*  Get maximum inline data capability */
    ret = ibv_query_qp(connid->qp, &qp_attr, IBV_QP_CAP, &qp_init_attr);
    if (ret)
    {
        throw std::runtime_error("QP query failed.");
    }
    max_inline = qp_attr.cap.max_inline_data;
}

int tsc_communicator_nb_rdma::get_transport_type()
{
    return TSC_TRANSPORT_RDMACM;
}

tsc_mr_h tsc_communicator_nb_rdma::register_mem(void *buf, size_t length, int flags)
{
    struct ibv_mr *mr;
    mr = ibv_reg_mr(connid->pd, (void *) buf, length, flags);
    if (!mr)
        throw std::runtime_error("Memory registration failed.");
    tsc_mr_h data;
    data.access_flags   = flags; 
    data.type           = TSC_TRANSPORT_RDMACM;
    data.native         = (void *) mr;
    data.addr           = mr->addr;
    return data;
}

void tsc_communicator_nb_rdma::deregister_mem(tsc_mr_h mr)
{
    if (mr.type != TSC_TRANSPORT_RDMACM)
        throw std::runtime_error("Invalid memory type!");

    int ret = ibv_dereg_mr((ibv_mr *) mr.native);
    if (ret)
        throw std::runtime_error(std::string("Deregistration failure: ")
                + std::string(strerror(errno)));
}

uint64_t tsc_communicator_nb_rdma::send_msg(tsc_mr_h mr, void *buf, size_t len)
{
    int ret;
    uint64_t ct = increment_sc();
    _tsc_send_ctx ctx = {};
    ret = rdma_post_send(connid, (void *) ct, buf, len, (ibv_mr *) mr.native, 0);
    if (ret)
        throw std::runtime_error(
                std::string("RDMA post send failed (SGE mode): ")
                + std::string(strerror(errno)));
    
    send_set.insert({ct, ctx});
    goto ok;

ok:
    send_pending++;
    return ct;
}

uint64_t tsc_communicator_nb_rdma::send_msg(void *buf, size_t length)
{
    int ret;
    uint64_t ct = increment_sc();
    _tsc_send_ctx ctx = {};
    if (max_inline <= length)
    {
        /*  If the message size is small enough, we can just send it inline. */
        ret = rdma_post_send(connid, (void *) ct, buf, length, 
                NULL, IBV_SEND_INLINE);
        if (ret)
        {
            std::runtime_error(
                std::string("RDMA post send failed (inline mode): ")
                + std::string(strerror(errno)));
        }
    }
    else
    {
        /*  Attempt to allocate a subregion of memory. */
        void *subr = snd_mgr->tmalloc(length);
        memcpy(subr, buf, length);
        if (!subr)
            goto try_reg;
        ctx.alloc_internal = subr;
        ret = rdma_post_send(connid, (void *) ct, subr, length, snd_mr, 0);
        if (ret)
        {
            throw std::runtime_error(
                    std::string("RDMA post send failed (SGE mode): ")
                    + std::string(strerror(errno)));
        }
    }

    goto ok;

try_reg:
    ctx.tmp_mr = rdma_reg_msgs(connid, buf, length);
    if (!ctx.tmp_mr)
        throw std::runtime_error("Memory allocation failed.");
    ret = rdma_post_send(connid, (void *) ct, buf, length, ctx.tmp_mr, 0);
    if (!ret)
        throw std::runtime_error(
            std::string("RDMA post send failed (SGE mode w/ reg):")
            + std::string(strerror(errno)));

ok:
    send_set.insert({ct, ctx});
    send_pending++;
    return ct;
}

uint64_t tsc_communicator_nb_rdma::recv_msg(tsc_mr_h mr, void *buf, size_t len)
{
    int ret;
    uint64_t ct = increment_rc();
    _tsc_recv_ctx ctx = {};
    ret = rdma_post_recv(connid, (void *) ct, buf, len, (ibv_mr *) mr.native);
    if (ret)
        throw std::runtime_error("Post receive failed.");

ok:

    recv_set.insert({ct, ctx});
    recv_pending++;
    return ct;
}

uint64_t tsc_communicator_nb_rdma::write_msg(tsc_mr_h local_mr, tsc_mr_h remote_mr,
        void *local_addr, void *remote_addr, size_t length)
{
    int ret;
    uint64_t ct = increment_sc();
    _tsc_send_ctx ctx = {};
    ret = rdmae_post_writeimm(connid, (void *) ct, local_addr, length,
            (ibv_mr *) local_mr.native, 0, (uint64_t) remote_addr, 
            ((ibv_mr *) remote_mr.native)->rkey, 0);
    if (ret)
        throw std::runtime_error(
                std::string("RDMA write with immediate failed: ")
                + std::string(strerror(errno)));

ok:
    send_set.insert({ct, ctx});
    send_pending++;
    return ct;
}

uint64_t tsc_communicator_nb_rdma::read_msg(tsc_mr_h local_mr, tsc_mr_h remote_mr,
        void *local_addr, void *remote_addr, size_t length)
{
    int ret;
    uint64_t ct = increment_sc();
    _tsc_send_ctx ctx = {};
    ret = rdma_post_read(connid, (void *) ct, local_addr, length,
            (ibv_mr *) local_mr.native, 0, (uint64_t) remote_addr,
            ((ibv_mr *) remote_mr.native)->rkey);
    if (ret)
        throw std::runtime_error(
                std::string("RDMA read failed: ")
                + std::string(strerror(errno)));

ok:
    send_set.insert({ct, ctx});
    send_pending++;
    return ct;
}

int tsc_communicator_nb_rdma::poll_sq(uint64_t *out, bool block)
{
    int ret;
    struct ibv_wc wc;
    if (!block)
    {
        ret = ibv_poll_cq(connid->send_cq, 1, &wc);
        if (ret < 0)
            throw std::runtime_error("Send CQ poll failed.");
        *out = wc.wr_id;
    }
    else
    {
        while ((ret = rdma_get_send_comp(connid, &wc)) == 0);
        if (ret < 0)
            throw std::runtime_error("Send CQ poll failed.");
        *out = wc.wr_id;
    }
    send_pending--;
    return ret;
}

int tsc_communicator_nb_rdma::poll_rq(uint64_t *out, bool block)
{
    int ret;
    struct ibv_wc wc;
    if (!block)
    {
        ret = ibv_poll_cq(connid->recv_cq, 1, &wc);
        if (ret < 0)
            throw std::runtime_error("Recv CQ poll failed.");
        *out = wc.wr_id;
        return ret;
    }
    else
    {
        while ((ret = rdma_get_recv_comp(connid, &wc)) == 0);
        if (ret < 0)
            throw std::runtime_error("Recv CQ poll failed.");
        *out = wc.wr_id;
    }
    recv_pending--;
    return ret;
}

/* ------------------ Server Transport Functions ------------------ */

/*
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


void tsc_server_rdma::init_server(const char *host, const char *port, int type, int backlog)
{
    struct rdma_addrinfo hints;
    struct ibv_qp_init_attr init_attr;

    init_attr.cap.max_send_wr      = 10;
    init_attr.cap.max_recv_wr      = 10;
    init_attr.cap.max_send_sge     = 10;
    init_attr.cap.max_recv_sge     = 10;
    init_attr.cap.max_inline_data  = 128;
    init_attr.sq_sig_all           = 1;

    // Passive (server) mode
    hints.ai_flags = RAI_PASSIVE;
    
    // Set reliability flags
    switch (type)
    {
        case SOCK_STREAM:
            init_attr.qp_type   = IBV_QPT_RC;
            hints.ai_port_space = RDMA_PS_TCP;
            break;
        case SOCK_DGRAM:
            init_attr.qp_type   = IBV_QPT_UD;
            hints.ai_port_space = RDMA_PS_UDP;
            break;
        default:
            throw std::runtime_error("Invalid protocol family.");
    }

    // Start server
    tsc_server_rdma::init_server(host, port, &hints, NULL, &init_attr, backlog);
}


tsc_rdma_id_h tsc_server_rdma::accept()
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

    tsc_rdma_id_h data;
    data.addr = (void *) new_id;
    return data;
}
*/

/* ------------------ Client Transport Functions ------------------ */

tsc_rdma_id_h tsc_util_rdmacm::connect(const char *host, const char *port, int type)
{
    
    struct rdma_cm_id *connid;
    tsc_rdma_id_h rdma_id;
    int ret;

    // Resolve route
    
    struct rdma_addrinfo hints = {};
    struct rdma_addrinfo *host_res;
    ret = rdma_getaddrinfo(host, port, &hints, &host_res);
    if (ret)
    {
        throw std::runtime_error("Could not resolve host.");
    }

    //  Configure connection parameters
    
    struct ibv_qp_init_attr init_attr = {};
    init_attr.cap.max_send_wr = 10;
    init_attr.cap.max_recv_wr = 10;
    init_attr.cap.max_send_sge = 10;
    init_attr.cap.max_recv_sge = 10;
    init_attr.sq_sig_all = 1;

    switch (type)
    {
        // This is actually more like SOCK_SEQPACKET but the definition isn't
        // available on all operating systems.
        case SOCK_STREAM:
            init_attr.qp_type = IBV_QPT_RC;
            break;
        // Warning: completely untested!!
        case SOCK_DGRAM:
            init_attr.qp_type = IBV_QPT_UD;
            break;
        default:
            throw std::runtime_error(
                std::string("Protocol family type ") 
                + std::to_string(type) + std::string("unknown."));
    }

    //  Create endpoint and connect to server

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

    rdma_id.addr = connid;
    return rdma_id;

}