/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Common RDMA Functions
    Copyright (C) 2021 Telescope Project
*/

#ifndef TSC_COMMON_H
#define TSC_COMMON_H

#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <cstdint>

/* Helper macros for common operations */

#define clear(x) memset(&x, 0, sizeof(x))                       // Clear fields of an existing variable
#define new(type, x) type x; memset(&x, 0, sizeof(x))           // Create a new zeroed item (on stack)
#define heapnew(type, x) type *x = calloc(1, sizeof(type))      // Create a new zeroed item (on heap)
#define println(x) printf(x "\n");                              // Print with new line...

/* Print a string as a series of hex bytes */

static inline void printb(char *buff, int n)
{
    for(int i=0; i < n; i++)
    {
        printf("%hhx", *(buff + i));
    }
    printf("\n");
}

/* Screen Capture Metadata */

typedef struct {
    enum tsc_capture_type capture_type;   // Screen capture provider (X11 etc.)
    uint32_t    width;          // Native width
    uint32_t    height;         // Native height
    uint32_t    fps;            // Native framerate
    uint16_t    bpp;            // Screen bits per pixel
    uint16_t    format;         // Screen pixel format
    union
    {
        struct {
            Display     *x_display;
            Window      x_window;
            int         x_screen_no;
        };
    };
} tsc_screen;

/* Initial Data Exchange Packets */

/*  The server first sends this hello packet to inform the client about its
    native pixel map and egress bandwidth available to service the user's 
    request.
*/
typedef struct {
    uint64_t    avail_bw;       // Bandwidth limit (bits/second)
    uint64_t    avail_mem;      // Memory limit (bytes)
    uint32_t    s_width;        // Native width
    uint32_t    s_height;       // Native height
    uint32_t    s_fps;          // Native framerate
    uint16_t    s_format;       // Screen pixel format
} __attribute__((__packed__)) T_ServerHello;

/*  The client then replies back with the format it wants from the server.
    If the client wants the server to send the native pixmap for the highest
    performance, the client can simply echo back the parameters it received 
    from the server.  
    The client is responsible for ensuring there is 
    enough available downstream bandwidth to handle the RDMA reads, as the
    server is not responsible for performing or throttling reads.
*/
typedef struct {
    uint32_t    s_width;        // Requested width
    uint32_t    s_height;       // Requested height
    uint32_t    s_fps;          // Requested framerate
    uint16_t    s_bpp;          // Screen bits per pixel
    uint16_t    s_format;       // Screen pixel format
    uint16_t    rdma_bufs;      // Number of buffers to use for page flipping.
} __attribute__((__packed__)) T_ClientHello;

/*  The server then performs memory allocation according to the client's
    request. If the request is invalid, it's indicated as such.
*/
typedef struct {
    uint32_t    ok_flag;        // If everything is ok, indicate 0xffffffff
    uint64_t    fb_addr;        // Frame buffer address
    uint64_t    fb_size;        // Frame buffer size
    uint64_t    st_addr;        // State buffer address
    uint64_t    st_size;        // State buffer size
    uint32_t    fb_rkey;        // Frame buffer RDMA key
    uint32_t    st_rkey;        // State buffer RDMA key
    uint32_t    fb_bufs;        // Frame buffer number of sub-buffers
} __attribute__((__packed__)) T_ServerBuffData;

/*  At this point, the connection is established. The client is responsible
    for telling the server which buffers need to be read.
*/

// Get the page-aligned size for a single frame
inline int paligned_fsize(int width, int height, int bpp)
{
    int ps = getpagesize();
    int raw_fsize = (width * height * (bpp / 8));
    return raw_fsize + (ps - (raw_fsize % ps));
}

/* Linked list containing information about every buffer
typedef struct {
    union {
        struct {
            XShmSegmentInfo     shminfo;        // Shared memory segment info
            uint32_t            bufid;          // Item number in buffer
            XImage              *ximage;        // X Image containing metadata
        };
    };
    T_ImageList *next;  // Next item in buffer
} T_ImageList;
*/

#endif