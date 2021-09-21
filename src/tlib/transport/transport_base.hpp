/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (Base Classes)
    Copyright (c) 2021 Telescope Project
*/

#pragma once

#include <netinet/in.h>
#include "transport.pb.h"

/* ------------------ Common Transport Functions ------------------ */

/* Available transport types in Telescope */
typedef enum {
    TSC_TRANSPORT_NONE      = 1,        // Dummy server
    TSC_TRANSPORT_TCP       = 1 << 1,   // TCP
    TSC_TRANSPORT_RDMACM    = 1 << 2,   // RDMA_CM + InfiniBand Verbs
    TSC_TRANSPORT_UCX       = 1 << 3    // Unified Communication X (UCX)
} tsc_transport_type;

/* Information needed to initialize the transport. */
struct tsc_transport {
    tsc_transport_type  flag;
    char                *host;
    char                *port;
    void                *attr;
};

class tsc_common_base {
    
public:
    
    void set_sockaddr(const char *address_str, const char *port, int is_server, struct sockaddr_in *out_addr);

    char *pack_msg(pb_wrapper *msg);

};

/* ------------------ Server Transport Functions ------------------ */

class tsc_server_base : public tsc_common_base {

public:

    bool is_ready();

    void init_server(const char *host, const char *port, int backlog);

private:

    int sockfd = -1;
    
};

/* ------------------ Client Transport Functions ------------------ */

class tsc_client_base : public tsc_common_base {

public:

    bool is_ready();
    
};