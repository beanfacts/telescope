/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Shared Memory Receive Functions
    Copyright (C) 2021 Telescope Project
*/

#include <stdio.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

// todo: support more resolutions ...
#define WIDTH   1920
#define HEIGHT  1080

struct shmimage
{
    XShmSegmentInfo shminfo;
    XImage *ximage;
    unsigned int *data;
};

void initimage(struct shmimage *image);
void destroyimage(Display *dsp, struct shmimage *image);
XImage *shm_create_ximage(Display *display, struct shmimage *image, int width, int height);
XImage *noshm_create_ximage(Display *display, Visual *visual, int width, int height, char *image);
int noshm_putimage(char *image, Display *display, Visual *visual, Window window, GC gc);