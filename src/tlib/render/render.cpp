/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Client Rendering Routines
    Copyright (c) 2021 Telescope Project
*/

#include <cstdio>

#include "common.hpp"
#include "render.hpp"



int tsc_render_context::init_context(int width, int height)
{
    return 1;
}

int tsc_render_context::init_x11_context(int width, int height)
{
    int ret;
    ret = trc_init_x11(1280, 720, 1);
    if (ret)
        return 1;
    return 0;
}

int tsc_render_context::x11_render_image(char *image, int width, int height, int bpp)
{
    
    XImage *cur = XCreateImage(x11_ctx.display, CopyFromParent,
        bpp, ZPixmap, 0, image, width, height, bpp, 0);
    if (!cur)
    {
        fprintf(stderr, "Could not create XImage.\n");
        return 1;
    }

    int ret = XPutImage(x11_ctx.display, x11_ctx.window, x11_ctx.gc,
        cur, 0, 0, 0, 0, width, height);
    if (ret)
    {
        fprintf(stderr, "Could not render image.\n");
        return 1;
    }

    return 0;
}

int tsc_render_context::trc_init_x11(int width, int height, int mt)
{
    if (mt)
    {
        Status mt_status = XInitThreads();
        if (!mt_status)
        {
            fprintf(stderr, "X11 thread init failed\n");
            return 1;
        }
    }

    x11_ctx.display = XOpenDisplay(NULL);
    if (!x11_ctx.display)
    {
        fprintf(stderr, "Could not open X display.\n");
        return 1;
    }

    int win_b_col = BlackPixel(x11_ctx.display, DefaultScreen(x11_ctx.display));
    int win_w_col = BlackPixel(x11_ctx.display, DefaultScreen(x11_ctx.display));
    
    x11_ctx.window = XCreateSimpleWindow(x11_ctx.display, 
            DefaultRootWindow(x11_ctx.display), 0, 0, width, height, 0, 
            win_b_col, 0x12345678 );

    x11_ctx.visual = DefaultVisual(x11_ctx.display, 0);

    XStoreName(x11_ctx.display, x11_ctx.window, "Telescope");
    XMapWindow(x11_ctx.display, x11_ctx.window);
    x11_ctx.gc = XCreateGC(x11_ctx.display, x11_ctx.window, 0, NULL);
    XFlush(x11_ctx.display);

    return 0;
}