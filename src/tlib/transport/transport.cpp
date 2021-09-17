/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library
    Copyright (c) 2021 Telescope Project
*/

// getaddrinfo
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <netdb.h>

#include "transport.hpp"
#include "transport_rdmacm.hpp"

/* ------------------ Common Transport Functions ------------------ */

/* ------------------ Server Transport Functions ------------------ */

void tsc_server::init_servers(struct tsc_transport *init_transport,
    struct tsc_transport *transports, int max_clients)
{
    
    int ret;
    int exit_flag = 0;


    // Create initial data exchange server.
    printf("Attempting to start listener on: %s:%s\n", init_transport->host, init_transport->port);
    server.meta_server.init_server(init_transport->host, init_transport->port, max_clients);

    // Initialise all the transports the user selected
    for (int i = 0;;i++)
    {
        switch (transports[i].flag)
        {
            case TSC_TRANSPORT_NONE:
            {
                exit_flag = 1;
                break;
            }
            case TSC_TRANSPORT_TCP:
            {
                server.tcp_server.init_server(transports[i].host, transports[i].port, max_clients);
                break;
            }
            case TSC_TRANSPORT_RDMACM:
            {
                server.rdma_server.init_server(transports[i].host, transports[i].port, max_clients);
                break;
            }
            case TSC_TRANSPORT_UCX:
            {
                throw std::runtime_error("UCX transport is not implemented.");
            }
        }
        if (exit_flag) { break; }
    }
}

/* ------------------ Client Transport Functions ------------------ */