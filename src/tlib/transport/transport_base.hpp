/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (Base Classes)
    Copyright (c) 2021 Telescope Project
*/

#include <netinet/in.h>

/* ------------------ Common Transport Functions ------------------ */

/* Available transport types in Telescope */
typedef enum {
    TSC_TRANSPORT_NONE      = 1,        // Dummy server
    TSC_TRANSPORT_TCP       = 1 << 1,   // TCP
    TSC_TRANSPORT_UCX       = 1 << 2,   // Unified Communication X (UCX)
    TSC_TRANSPORT_RDMACM    = 1 << 3    // RDMA_CM + InfiniBand Verbs
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

    void init_client(const char *host, const char *port, tsc_transport_type transport_type);

    void set_sockaddr(const char *address_str, const char *port, int is_server, struct sockaddr_in *out_addr);

private:

    int sockfd = -1;
    
};