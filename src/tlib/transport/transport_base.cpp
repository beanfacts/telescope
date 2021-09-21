/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (Base Classes)
    Copyright (c) 2021 Telescope Project
*/

#include "transport_base.hpp"

#include <sys/socket.h>
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // inet_aton

#include <stdexcept>    // runtime_error
#include <errno.h>      // errno
#include <cstring>      // memset

void tsc_common_base::set_sockaddr(const char *address_str, const char *port, int is_server, struct sockaddr_in *out_addr)
{
    int ret;
    in_addr raw_addr;

    ret = inet_aton(address_str, &raw_addr);
    if (!ret)
    {
        errno = EINVAL;
        throw std::runtime_error("Cannot convert address string to integer");
    }

    char *endptr;
    errno = 0;
    long _port = strtol(port, &endptr, 10);
    if (_port > 65535 || _port < 1)
    {
        errno = EINVAL;
        throw std::runtime_error("Invalid port provided");
    }
    else if (errno)
    {
        throw std::runtime_error(std::string("Invalid port info: ") + std::string(strerror(errno)));
    }
    
    memset(out_addr, 0, sizeof(*out_addr));
    out_addr->sin_family    = AF_INET;
    out_addr->sin_addr      = raw_addr;
    out_addr->sin_port      = htons(_port);
}


/* ---- Protobuf handling functions ---- */

char *tsc_common_base::pack_msg(pb_wrapper *msg)
{
    // We do not have messages larger than 2GB. But we check it
    // anyway in case the message data is malformed
    long lbufsize = msg->ByteSizeLong();
    uint32_t maxbufsize = -1;
    if (lbufsize >= maxbufsize) std::runtime_error("boot too big");
    static_assert(sizeof(uint32_t) == 4);
    uint32_t bufsize = (uint32_t) lbufsize;
    // Pack message size into message contents
    char *buf = (char *) malloc(bufsize + 4);
    bool status = msg->SerializeToArray(buf + 4, bufsize);
    if (!status) throw std::runtime_error("Serialize failed.");
    * (uint32_t *) buf = htonl(bufsize);
    return buf;
}