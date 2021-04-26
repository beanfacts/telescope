#include "screencap.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


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


int createimage(Display *dsp, struct shmimage *image, int width, int height, int bpp)
{

    int ret;

    // Allocate the memory required for storing the X11 image
    image->shminfo.shmid = shmget(IPC_PRIVATE, width * height * bpp, IPC_CREAT | 0600);
    if (image->shminfo.shmid == -1)
    {
        fprintf(stderr, "Error allocating X shared memory segment.\n");
        return false;
    }

    // Find the memory region of the shared memory segment
    image->shminfo.shmaddr = shmat(image->shminfo.shmid, 0, 0);
    if (image->shminfo.shmaddr == (char *) -1)
    {
        fprintf(stderr, "Error allocating X shared memory segment.\n");
        return false;
    }

    // Delete the shared memory segment on exit or crash
    ret = shmctl(image->shminfo.shmid, IPC_RMID, 0);
    if (ret == -1)
    {
        fprintf(stderr, "Error marking the segment for deletion.\n", strerror(errno));
    }

    // If X11 forwarding is used, the attachment may fail since the display
    // isn't using the same memory region
    //image->data = (unsigned int *) image->shminfo.shmaddr;
    image->shminfo.readOnly = false;
    
    // Finally, tell X11 to use the shared memory segment
    ret = XShmAttach(dsp, &image->shminfo);
    if (!ret) {
        fprintf(stderr, "Could not attach X display (is this a remote display?)\n");
    }

    XSync(dsp, false);

    image->ximage = XShmCreateImage(
                        dsp,
                        XDefaultVisual(dsp, XDefaultScreen(dsp)),
                        DefaultDepth(dsp, XDefaultScreen(dsp)),
                        ZPixmap,
                        0,
                        &image->shminfo,
                        0,
                        0);
    
    if(!image->ximage)
    {
        destroyimage(dsp, image);
        printf("Could not allocate memory for the XImage structure.\n");
        return false;
    }

    image->ximage->data = (char *) image->shminfo.shmaddr;
    image->ximage->width = width;
    image->ximage->height = height;

    ret = XShmAttach(dsp, &image->shminfo);
    if (!ret)
    {
        fprintf(stderr, "Could not attach SHM segment\n");
    }
    
    return true;
}

// Capture an image from the root window. This will fail for big endian but i dont care
void getrootwindow(Display *dsp, struct shmimage *image, int screen_no, int screen_width)
{
    int ret;
    printf("Passed in: %d\n", image->ximage->data);
    printb(image->ximage->data, 50);
    if (screen_no == 0)
    {
        ret = XShmGetImage(dsp, XDefaultRootWindow(dsp), image->ximage, 0, 0, AllPlanes);
        if (ret)
        {
            fprintf(stderr, "X11 Cap Error: %d\n", ret);
        }
    }
    else
    {
        ret = XShmGetImage(dsp, XDefaultRootWindow(dsp), image->ximage, screen_width * screen_no, 0, AllPlanes);
        if (ret)
        {
            fprintf(stderr, "X11 Cap Error: %d\n", ret);
        }
    }
    printf("Got out\n");
    printb(image->ximage->data, 50);
}

// If there are multiple screens, this will create a copy that can be sent
// over RDMA.
/*
int get_frame(struct shmimage * src, struct shmimage * dst)
{
    int sw = src->ximage->width;
    int sh = src->ximage->height;
    int dw = dst->ximage->width;
    int dh = dst->ximage->height;

    unsigned int * d = dst->data;
    int j, i;
    for(j = 0; j < dh; ++j)
    {
        for(i = 0; i < dw; ++i)
        {
            int x = (float)(i * src->ximage->width) / (float)dw;
            int y = (float)(j * src->ximage->height) / (float)dh;
            *d++ =  src->data[ y * src->ximage->width + x ];
        }
    }
    return true;
}
*/