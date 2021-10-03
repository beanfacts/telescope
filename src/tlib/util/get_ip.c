/*
    SPDX-License-Identifier: MIT
    DPU_MPI IP Address Helper Functions
    Copyright (c) 2021 Tim Dettmar
*/

#include "get_ip.h"

#ifdef __cplusplus
extern "C" {
#endif

char *find_addr(char *ifname)
{
    struct ifaddrs *ifaddrs;
    getifaddrs(&ifaddrs);
    do
    {
        if (strcmp(ifaddrs->ifa_name, ifname) == 0 && ifaddrs->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *addr = (struct sockaddr_in *) ifaddrs->ifa_addr;
            return inet_ntoa(addr->sin_addr);
        }
        else
        {
            ifaddrs = ifaddrs->ifa_next;
        }
    }
    while (ifaddrs->ifa_next);
    return NULL;
}

#ifdef __cplusplus
}
#endif