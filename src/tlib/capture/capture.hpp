/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Desktop Capture Library
    Copyright (c) 2021 Telescope Project
*/

#ifndef T_CAPTURE_H
#define T_CAPTURE_H


/* Screen Capture Libraries */
#include <X11/Xlib.h>               // Base X11 library
#include <X11/extensions/XShm.h>    // Shared memory extensions
#include <X11/extensions/Xrandr.h>  // Get FPS

#include <vector>

/* Internal Libraries */
#include "common.hpp"

struct tsc_shminfo {
    tsc_capture_type    type;       // Capture method
    int                 shmid;      // System SHM id
    void                *addr;      // Address
    uint64_t            length;     // Buffer length
    union
    {
        XImage          *x_image;   // X11 specific data
    };
    void                *mr_ctx;    // Memory registration context (e.g. ibv_mr *)
    void                *context;   // User context
};

class tsc_capture_context {

public:

    void init(tsc_screen *scr);

    /* Initialize the capture system by allocating memory. */
    void start_capture(uint32_t width, uint32_t height, uint32_t bpp, uint32_t fps, uint32_t buffers);

    /* Update the frame buffer with specified index. */
    void update_frame(int index);

    /* Get the memory address of the frame. */
    void *get_frame_addr(int index, int *size);

    /* Get the number of frame buffers available to service clients. */
    int get_buf_count();
    
    /* Get complete listing of shared memory segments. */
    std::vector<tsc_shminfo> get_shminfo();


private:

    std::vector<tsc_shminfo>    shminfo;
    tsc_screen                  screen;

    void init_context();

};

#endif