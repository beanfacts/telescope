/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Shared Memory Receive Functions
    Copyright (C) 2021 Telescope Project
*/

#include "receive_shm.h"

void initimage(struct shmimage *image)
{
    image->ximage = NULL;
    image->shminfo.shmaddr = (char *) -1;
}


void destroyimage(Display *dsp, struct shmimage *image)
{
    if (image->ximage)
    {
        XShmDetach(dsp, &image->shminfo);
        XDestroyImage(image->ximage);
        image->ximage = NULL;
    }

    if (image->shminfo.shmaddr != (char *) -1)
    {
        shmdt(image->shminfo.shmaddr);
        image->shminfo.shmaddr = (char *) -1;
    }
}


XImage *shm_create_ximage(Display *display, struct shmimage *image, int width, int height)
{
    
    image->shminfo.shmid = shmget(IPC_PRIVATE, width * height * 4, IPC_CREAT | 0600);
    if (image->shminfo.shmid == -1)
    {
        perror("Receive SHM");
        return false;
    }

    image->shminfo.shmaddr = (char *) shmat(image->shminfo.shmid, 0, 0);
    if(image->shminfo.shmaddr == (char *) -1)
    {
        perror("Receive SHM");
        return false;
    }


    image->data = (unsigned int *) image->shminfo.shmaddr;
    image->shminfo.readOnly = false;

    shmctl(image->shminfo.shmid, IPC_RMID, 0);

    image->ximage = XShmCreateImage(display, XDefaultVisual(display, XDefaultScreen(display)),
                        DefaultDepth(display, XDefaultScreen(display)), XYPixmap, 0,
                        &image->shminfo, 0, 0);
    if(!image->ximage)
    {
        destroyimage(display, image);
        perror("XImage structure allocation");
        return false;
    }

}

// Functions for creating X images
// return XShmCreateImage(display, visual, 24, ZPixmap, 0, image, width, height, 32, 0);
// return XCreateImage(display, visual, 24, ZPixmap, 0, image, width, height, 32, 0);

XImage *noshm_create_ximage(Display *display, Visual *visual, int width, int height, char *image)
{
    return XCreateImage(display, visual, 24,ZPixmap, 0, image, width, height, 32, 0);
}


int noshm_putimage(char *image, Display *display, Visual *visual, Window window, GC gc)
{    
    XImage *ximage = noshm_create_ximage(display, visual, WIDTH, HEIGHT, image);
    XEvent event;
    bool exit = false;
    int r = XPutImage(display, window, gc, ximage, 0, 0, 0, 0, WIDTH, HEIGHT);
    return 0;
}