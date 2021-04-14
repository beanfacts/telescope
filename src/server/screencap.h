#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifndef SCREENCAP_H_
#define SCREENCAP_H_

struct shmimage
{
    XShmSegmentInfo shminfo ;
    XImage * ximage ;
    unsigned int * data ; // will point to the image's BGRA packed pixels
};

void initimage(struct shmimage* image);
void destroyimage(Display* dsp, struct shmimage* image);
int createimage(Display* dsp, struct shmimage* image, int width, int height);
void getrootwindow(Display* dsp, struct shmimage* image, int screen_no, int screen_width);
int get_frame(struct shmimage* src, struct shmimage* dst);

#endif
