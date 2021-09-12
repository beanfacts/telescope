/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Common RDMA Functions
    Copyright (c) 2021 Telescope Project
*/

#ifndef TSC_COMMON_H
#define TSC_COMMON_H

#define TELESCOPE_VERSION "0.2"

#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <cstdint>

/* Screen Capture Libraries */
#include <X11/Xlib.h>               // Base X11 library
#include <X11/extensions/XShm.h>    // Shared memory extensions
#include <X11/extensions/Xrandr.h>  // Get FPS

#include "errors.hpp"


typedef enum {
    TSC_CAPTURE_NONE        = 0,
    TSC_CAPTURE_X11         = 1,
    TSC_CAPTURE_PW          = 2,
    TSC_CAPTURE_NVFBC       = 3,
    TSC_CAPTURE_AMF         = 4,
    TSC_CAPTURE_LGSHM       = 5
} tsc_capture_type;

/* Helper macros for common operations */

#define _tsc_clear(x) memset(&x, 0, sizeof(x))                      // Clear fields of an existing variable
#define _tsc_calloc(type, x) type *x = calloc(1, sizeof(type))      // Create a new zeroed item (on heap)
#define _tsc_println(x) printf(x "\n");                             // Print with new line...

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
    tsc_capture_type capture_type;   // Screen capture provider (X11 etc.)
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

// Get the page-aligned size for a single frame
inline int tsc_memalign_frame(int width, int height, int bpp)
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