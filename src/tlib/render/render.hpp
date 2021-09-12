/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Client Rendering Routines
    Copyright (c) 2021 Telescope Project
*/

#ifndef T_RENDER_H
#define T_RENDER_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cstdio>

class tsc_render_context {

public:
    
    /*
        Initialise the Telescope window with a specific size.
        The initial size can show debug info etc. but it will likely
        be resized when a connection is established.
    */
    int init_context(int width, int height);

    /*
        Legacy function: Initialise an X11 window.
        This code is copied over from the original Telescope version,
        but is deprecated in favour of using a cross-platform
        graphics library (might be GLFW or something else, tbd).
    */
    int init_x11_context(int width, int height);

    /*
        Legacy function: Render image.
        This version does not use shared memory extensions, but it
        will be deprecated anyway.
    */
    int x11_render_image(char *image, int width, int height, int bpp);

private:

    struct _x11_ctx {
        int             dpy_width;
        int             dpy_height;
        int             dpy_bpp;
        Display         *display;
        Visual          *visual;
        Window          window;
        GC              gc;
        XImage          **ximgs;
    };

    struct _x11_ctx     x11_ctx;

    int trc_init_x11(int width, int height, int mt);

};


#endif