/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (TCP)
    Copyright (c) 2021 Telescope Project
*/

#include "transport_tcp.hpp"
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

/* ------------------ Common Transport Functions ------------------ */

/* ------------------ Server Transport Functions ------------------ */

int tsc_server_tcp::is_ready()
{
    return (tsc_server_tcp::sockfd > 0);
}

void tsc_server_tcp::init_server(const char *host, const char *port, int backlog)
{
    struct sockaddr_in out_addr;
    tsc_server_tcp::set_sockaddr(host, port, 1, &out_addr);
    tsc_server_tcp::init_server(&out_addr, 1);
}

void tsc_server_tcp::init_server(struct sockaddr_in *sa, int backlog)
{
    tsc_server_tcp::sockfd = get_server(sa, backlog);
}
    
int tsc_server_tcp::get_server(struct sockaddr_in *sa, int backlog)
{
    int ret;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        throw std::runtime_error("Socket creation failed");
    }

    ret = bind(sock, (struct sockaddr *) sa, sizeof(*sa));
    if (ret)
    {
        throw std::runtime_error("Socket bind failed");
    }

    ret = listen(sock, backlog);
    if (ret < 0)
    {
        throw std::runtime_error("Socket listen failed");
    }

    return sock;
}

void tsc_server_tcp::attach_fd(int _fd)
{
    sockfd = _fd;
}

int tsc_server_tcp::accept()
{
    int fd;
    sockaddr sa;
    socklen_t sa_len;

    fd = ::accept(tsc_server_tcp::sockfd, &sa, &sa_len);
    if (fd < 0)
    {
        throw std::runtime_error("Could not accept client.");
    }

    return fd;
}

/* ------------------ Client Transport Functions ------------------ */

void tsc_client_tcp::connect(const char *host, const char *port)
{
    int ret;
    int sfd = -1;

    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    
    struct addrinfo *result, *rp;
    ret = getaddrinfo(host, port, &hints, &result);
    if (ret != 0) throw std::runtime_error("getaddrinfo failed.");

    for (rp = result; rp != nullptr; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (::connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;

        ::close(sfd);
    }

    freeaddrinfo(result);

    if (rp == nullptr) throw std::runtime_error("Connection failed.");
    tsc_client_tcp::sockfd = sfd;

}