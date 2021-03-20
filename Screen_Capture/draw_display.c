#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "screencap.h"
#include "screencap.c"
#include "screen_struc.h"
#include <sys/time.h>
#include <unistd.h>

void getrootwindow(Display* dsp, struct shmimage* image)
{
    XShmGetImage(dsp, XDefaultRootWindow(dsp), image->ximage, 0, 0, AllPlanes);
}


Window createwindow(Display* dsp, int width, int height)
{
    unsigned long mask = CWBackingStore;
    XSetWindowAttributes atr;
    Window window = XCreateWindow(dsp, DefaultRootWindow(dsp),0, 0, width, height, 0,DefaultDepth(dsp, XDefaultScreen(dsp)),InputOutput, CopyFromParent, mask, &atr);

    XStoreName(dsp, window, "Screen Capture");
    XSelectInput(dsp, window, StructureNotifyMask);
    XMapWindow(dsp, window);
    return window;
}
int getframe(struct shmimage* src, struct shmimage* dst)
{   

    int sw = src->ximage->width;
    int sh = src->ximage->height;
    int dw = dst->ximage->width;
    int dh = dst->ximage->height;

    int* d = dst->data;
    int j, i;
    for(j = 0; j < dh; ++j)
    {
        for(i = 0; i < dw; ++i)
            int x =(i* src->ximage->width) / dw;
            int y =(j* src->ximage->height) / dh;
            *d++ = src->data[ y* src->ximage->width + x ];
        }
    }
    return true;
}

int run(Display* dsp, Window window, struct shmimage* src, struct shmimage* dst)
{
    XGCValues xgcvalues;
    GC gc = XCreateGC(dsp, window, GCGraphicsExposures, &xgcvalues);

    Atom delete_atom = XInternAtom(dsp, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dsp, window, &delete_atom, True);

    XEvent xevent;
    int running = true;
    int initialized = false;
    int dstwidth = dst->ximage->width;
    int dstheight = dst->ximage->height;
    long frames = 0;
    int fd = ConnectionNumber(dsp);
    while(running)
    {
        while(XPending(dsp))
        {
            XNextEvent(dsp, &xevent);
            if((xevent.type == ClientMessage && xevent.xclient.data.l[0] == delete_atom)
                || xevent.type == DestroyNotify)
            {
                running = false;
                break;
            }
            else if(xevent.type == ConfigureNotify)
            {
                if(xevent.xconfigure.width == dstwidth
                    && xevent.xconfigure.height == dstheight)
                {
                    initialized = true;
                }
            }
        }
        if(initialized)
        {
            getrootwindow(dsp, src);
            getframe(src, dst);
            XShmPutImage(dsp, window, gc, dst->ximage, 0, 0, 0, 0, dstwidth, dstheight, False);
        }
    }
    return true;
}

int main(int argc, char* argv[])
{
    Display* dsp = XOpenDisplay(NULL);//create connection to the X server
    int screen = XDefaultScreen(dsp);
    if(!dsp)
    {
        printf("Screen Capture" ": could not open a connection to the X server\n");
        return 1;
    }

    (!XShmQueryExtension(dsp));// check the avalibility of the SHM

    struct shmimage src, dst;
    initimage(&src);
    int width = XDisplayWidth(dsp, screen);
    int height = XDisplayHeight(dsp, screen);
    
    initimage(&dst);
    int dstwidth = width / 2;
    int dstheight = height / 2;
    createimage(dsp, &src, width, height);
    createimage(dsp, &dst, dstwidth, dstheight);

    Window window = createwindow(dsp, dstwidth, dstheight);
    run(dsp, window, &src, &dst);
    XDestroyWindow(dsp, window); 

    destroyimage(dsp, &src);
    destroyimage(dsp, &dst);
    XCloseDisplay(dsp);
    return 0;
}