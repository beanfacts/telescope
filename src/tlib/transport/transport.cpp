/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library
    Copyright (c) 2021 Telescope Project
*/

// getaddrinfo
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netdb.h>

#include "transport.hpp"
#include "transport_rdmacm.hpp"

/* ------------------ Common Transport Functions ------------------ */

void printb(char *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        printf("%hhx ", *(char *) (buf + i));
    }
    printf("\n");
}

void comp_receive(void *buf, int fd, ssize_t bytes)
{
    ssize_t recvd;
    ssize_t total = 0;
    while (bytes > 0)
    {
        recvd = recv(fd, (char *) buf + total, bytes, 0);
        if (recvd <= 0)
        {
            throw std::runtime_error("Receive failed.");
        }
        else
        {
            bytes -= recvd;
            total += recvd;
        }
    }
}

void comp_send(void *buf, int fd, ssize_t bytes)
{
    ssize_t sent;
    ssize_t total = 0;
    while (bytes > 0)
    {
        sent = send(fd, (char *) buf + total, bytes, 0);
        if (sent <= 0)
        {
            throw std::runtime_error("Send failed.");
        }
        else
        {
            bytes -= sent;
            total += sent;
        }
    }
}

tsc_mbuf tsc_recv_msg(int fd)
{
    uint32_t numbytes;
    tsc_mbuf buf;
    
    // Get the incoming message size in network byte order
    std::cout << "Getting message size...\n";
    comp_receive(&numbytes, fd, 4);
    numbytes = ntohl(numbytes);
    
    // Should check these parameters
    buf.size = numbytes;

    // Convert incoming message to host byte order
    buf.buf = (char *) calloc(1, numbytes);
    if (!buf.buf) throw std::runtime_error("Memory allocation failed.");
    comp_receive(buf.buf, fd, numbytes);
    return buf;
}

void tsc_send_msg(char *buf, int fd)
{
    uint32_t bufsize = ntohl(((uint32_t *) buf)[0]);
    comp_send(buf, fd, bufsize + 4);
}

/* ------------------ Server Transport Functions ------------------ */

void tsc_server::init_servers(struct tsc_transport *init_transport,
    struct tsc_transport *transports, int max_clients)
{
    
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

tsc_client_repr tsc_server::get_client()
{
    std::cout << "Waiting for client...\n";
    int fd = tsc_server::server.meta_server.accept();
    tsc_client_repr repr;
    repr.attach_init_fd(fd);
    return repr;
}

void tsc_server::add_client_channel(tsc_client_repr *client, int transport_flag)
{
    if (transport_flag == TSC_TRANSPORT_RDMACM)
    {
        struct rdma_cm_id *id = tsc_server::server.rdma_server.accept();
        if (id == nullptr)
            throw std::runtime_error("Failed to accept client.");
        client->attach_cm_id(id);
    }
}

/* ------------------ Client Transport Functions ------------------ */


void tsc_client::init_control_channel(const char *host, const char *port)
{
    std::cout << "before: " << tsc_client::client.meta_server.sockfd << "\n";
    tsc_client::client.meta_server.connect(host, port);
    std::cout << "after: " << tsc_client::client.meta_server.sockfd << "\n";
}


void tsc_client::init_transport(struct tsc_transport *transport)
{
    if (transport->flag == TSC_TRANSPORT_RDMACM)
    {
        tsc_client::client.rdma_server.connect(transport->host, transport->port);
    }
}


tsc_mbuf tsc_client::recv_msg()
{
    int fd = tsc_client::client.meta_server.sockfd;
    if (fd < 0) throw std::runtime_error("Transfer function called with invalid fd");
    return tsc_recv_msg(fd);
}


void tsc_client::send_msg(char *buf)
{
    int fd = tsc_client::client.meta_server.sockfd;
    if (fd < 0) throw std::runtime_error("Transfer function called with invalid fd");
    tsc_send_msg(buf, fd);
}