/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Desktop Capture Library
    Copyright (c) 2021 Telescope Project
*/


#include "capture.hpp"

int tsc_init_capture(tsc_screen *scr)
{

    int ret;
    
    if (scr->capture_type == TSC_CAPTURE_X11)
    {
        // Prepare X server connection for multithreading
        ret = XInitThreads();
        if (!ret){
            fprintf(stderr, "X11 threading error: %d\n", ret);
            return 1;
        }

        // Grab the root display
        // On some X servers and Xwayland, this will return a black screen.
        scr->x_display = XOpenDisplay(NULL);
        if(!scr->x_display)
        {
            fprintf(stderr, "Could not open a connection to the X server\n");
            return 1;
        }

        scr->x_window = XDefaultRootWindow(scr->x_display);
        scr->x_screen_no = 0;

        // Check the X server supports shared memory extensions
        if(!XShmQueryExtension(scr->x_display))
        {
            XCloseDisplay(scr->x_display);
            fprintf(stderr, "Your X server does not support shared memory extensions.\n");
            return 1;
        }

        scr->width  = XDisplayWidth(scr->x_display, scr->x_screen_no);
        scr->height = XDisplayHeight(scr->x_display, scr->x_screen_no);

        XRRScreenConfiguration *xrr = XRRGetScreenInfo(scr->x_display, scr->x_window);
        scr->fps = (uint32_t) XRRConfigCurrentRate(xrr);

        double bw_frame = (double) scr->width * (double) scr->height * (double) scr->fps * (double) 32.0;

        printf( "Capturing X Screen %d (%d x %d @ %d Hz)\n"
                "Estimated bandwidth: %.3f Gbps\n",
                 scr->x_screen_no, scr->width, scr->height, scr->fps,
                 (bw_frame / (double) (1 << 30)));

        return 0;
    }

    fprintf(stderr, "Invalid capture function.\n");
    return 1;

}