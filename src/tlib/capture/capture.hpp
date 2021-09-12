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

/* Internal Libraries */
#include "common.hpp"


/*
    Setup the capture system.
    tsc_screen *screen -> should have capture_type populated.
*/
int tsc_init_capture(tsc_screen *scr);

#endif