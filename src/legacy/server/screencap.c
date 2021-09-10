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

// Clear the pointer values but do not allocate memory
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

// Allocate a memory segment shared between the X server and the client machine.
int createimage(Display *dsp, T_ImageList *imlist, int width, int height, int bpp, int frames)
{

    // Invalid parameter check
    if (frames < 1 || width < 1 || height < 1 || bpp < 1 || dsp == 0)
    {
        return false;
    }
    
    // Both RDMA and inter-process shared memory require page-aligned pointers.
    // First, the frame size is aligned to the page size and then the memory is
    // allocated (also aligned to the page size). It's not necessary to perform
    // this check twice, but I will do it anyway.
    void *shmem = 0;
    int fsize   = paligned_fsize(width, height, bpp);
    if (fsize)
    {
        shmem = aligned_alloc(getpagesize(), fsize * frames);
    }
    
    // Allocate memory to store the image list
    T_ImageList *imlist = calloc(1, sizeof(T_ImageList) * frames);
    T_ImageList *imnode;

    // Attach a shared memory section for each frame. This does seem like an
    // inefficient way to do it, but I do not have any clear documentation
    // about how to use MIT-SHM.
    for (int i = 0; i < frames; i++)
    {
        
        // Change the node reference
        imnode = imlist + (sizeof(T_ImageList) * i);

        // Get a new shared memory identifier
        imnode->shminfo.shmid = shmget(IPC_PRIVATE, fsize, IPC_CREAT | 0600);
        if (imnode->shminfo.shmid == -1)
        {
            fprintf(stderr, "Error allocating X shared memory segment.\n");
            return false;
        }

        // Find the memory region of the shared memory segment
        imnode->shminfo.shmaddr = shmat(imnode->shminfo.shmid, shmem, 0);
        if (imnode->shminfo.shmaddr == (char *) -1)
        {
            fprintf(stderr, "Error allocating X shared memory segment: %s\n", strerror(errno));
            goto error;
        }
        else if (imnode->shminfo.shmaddr != shmem)
        {
            fprintf(stderr, "Shared memory pointer mismatch (possible page misalignment). Cannot continue.\n");
            goto error;
        }

        // Delete the shared memory segment on exit or crash
        int ret = shmctl(imnode->shminfo.shmid, IPC_RMID, 0);
        if (ret == -1)
        {
            fprintf(stderr, "Error marking the segment for deletion: %s\n", strerror(errno));
            goto error;
        }

        // Tell X11 to use the shared memory segment
        imnode->shminfo.readOnly = false;
        ret = XShmAttach(dsp, &imnode->shminfo);
        if (!ret) {
            fprintf(stderr, "Could not attach X display (is this a remote display?)\n");
            goto error;
        }
        
        // Don't know if needed
        XSync(dsp, false);

        // Create XImage and allocate memory from allocated SHM region
        imnode->ximage = XShmCreateImage(
            dsp,
            XDefaultVisual(dsp, XDefaultScreen(dsp)),
            DefaultDepth(dsp, XDefaultScreen(dsp)),
            ZPixmap,
            0,
            &imnode->shminfo,
            0,
            0
        );
        if (!imnode->ximage)
        {
            fprintf(stderr, "XShm image creation failed.\n");
            goto error;
        }
        else
        {
            imnode->ximage->data = (char *) imnode->shminfo.shmaddr;
            imnode->ximage->width = width;
            imnode->ximage->height = height;
        }

        // Set the pointer to point to the next node
        // (but in this case we allocated it contiguously)
        imnode->next = imnode + sizeof(T_ImageList);

    }

    // Tail node
    imnode->next = NULL;
    return true;

    // Clean up on error
    error:
        free((void *) imlist);
        free(shmem);
        imlist = 0;
        return false;

}

// Capture an image from the root window. While it should work for all machines, I don't
// have any machines that still use big endian format. This is not the most efficient
// use of bandwidth, as it will transmit all 32 bits when only 24 are needed to display
// the image on screen.
void getrootwindow(Display *dsp, struct shmimage *image, int offset_x)
{
    int ret;
    printf("Passed in: %d\n", image->ximage->data);
    printb(image->ximage->data, 50);
    ret = XShmGetImage(dsp, XDefaultRootWindow(dsp), image->ximage, offset_x, 0, AllPlanes);
    if (ret)
    {
        fprintf(stderr, "X11 Cap Error: %d\n", ret);
    }
    printf("Got out\n");
    printb(image->ximage->data, 50);
}