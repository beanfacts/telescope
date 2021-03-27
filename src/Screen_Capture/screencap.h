#pragma once
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "screen_struc.h"

void initimage( struct shmimage * image );

void destroyimage( Display * dsp, struct shmimage * image );

int createimage( Display * dsp, struct shmimage * image, int width, int height );

void getrootwindow( Display * dsp, struct shmimage * image ,int screen_no,int screen_width);

int get_frame( struct shmimage * src, struct shmimage * dst );
