/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library
    Common Address Manipulation Functions
*/

#include <arpa/inet.h>  // htons
#include <sys/socket.h> // sockaddr_in

/*
    Common function to set the sockaddr struct for connection.
    This struct can then be used by any transport that uses this struct,
    such as UCX, RDMA, TCP, etc.
    Currently IPv4 only.
*/
void set_sockaddr(const char *address_str, const uint16_t port, struct sockaddr_in *out_addr, int is_server);