/*
    SPDX-License-Identifier: MIT
    DPU_MPI IP Address Helper Functions
    Copyright (c) 2021 Tim Dettmar
*/

#include <sys/types.h>
#include <ifaddrs.h>
#include <string.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

char *find_addr(char *ifname);


#ifdef __cplusplus
}
#endif