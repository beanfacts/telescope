/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library
    Common Address Manipulation Functions
*/

#include "transport_common.h"

void set_sockaddr(const char *address_str, const uint16_t port, struct sockaddr_in *listen_addr, int is_server)
{
    memset(listen_addr, 0, sizeof(struct sockaddr_in));
    listen_addr->sin_family      = AF_INET;
    listen_addr->sin_addr.s_addr = (address_str) ? inet_addr(address_str) : INADDR_ANY;
    listen_addr->sin_port        = htons(port);
}