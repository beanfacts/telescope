/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Common RDMA Functions
    Copyright (c) 2021 Telescope Project
*/

#ifndef TSC_COMMON_H
#define TSC_COMMON_H

/*  Telescope version string.
    - For point releases, use SemVer
    - For commits, use time/date (UTC) */
#define TELESCOPE_VERSION "2021.10.03-15.32"

#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <cstdint>

/*  Telescope capture types. */
typedef enum {
    TSC_CAPTURE_X11         = 1,
    TSC_CAPTURE_PW          = (1 << 1),
    TSC_CAPTURE_NVFBC       = (1 << 2),
    TSC_CAPTURE_AMF         = (1 << 3),
    TSC_CAPTURE_LGSHM       = (1 << 4)
} tsc_capture_type;

/* Print a string as a series of hex bytes */

static inline void printb(char *buff, int n)
{
    for(int i=0; i < n; i++)
    {
        printf("%hhx", *(buff + i));
    }
    printf("\n");
}

#endif