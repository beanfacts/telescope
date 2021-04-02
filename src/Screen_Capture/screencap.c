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

#define BPP    4 // do not hardcode

/* todo: fix! ugly function space!!!! */
void initimage( struct shmimage * image )
{
    image->ximage = NULL ;
    image->shminfo.shmaddr = (char *) -1 ;
}

void destroyimage( Display * dsp, struct shmimage * image )
{
    if( image->ximage )
    {
        XShmDetach( dsp, &image->shminfo ) ;
        XDestroyImage( image->ximage ) ;
        image->ximage = NULL ;
    }

    if( image->shminfo.shmaddr != ( char * ) -1 )
    {
        shmdt( image->shminfo.shmaddr ) ;
        image->shminfo.shmaddr = ( char * ) -1 ;
    }
}


int createimage( Display * dsp, struct shmimage * image, int width, int height )
{
    
    /* Get the X11 display parameters to allocate the required amount of memory */
    unsigned int disp_loc_x, disp_loc_y, disp_width, disp_height, disp_bpp, disp_border = 0;
    XGetGeometry(dsp, XDefaultRootWindow(dsp), NULL, &disp_loc_x, &disp_loc_y,
        &disp_width, &disp_height, &disp_border, &disp_bpp);

    printf( "Display parameters:\n"
            " X Position   : %u\n Y Position   : %u\n"
            " Disp. Width  : %u\n Disp. Height : %u\n Disp. Depth  : %u\n"
            " Disp. Border : %u\n",
            disp_loc_x, disp_loc_y, disp_width, disp_height, disp_bpp, disp_border
        );
   
    // Create a shared memory area 
    image->shminfo.shmid = shmget( IPC_PRIVATE, width * height * BPP, IPC_CREAT | 0600 ) ;
    if( image->shminfo.shmid == -1 )
    {
        perror( "Error" ) ;
        return false ;
    }

    // Map the shared memory segment into the address space of this process
    image->shminfo.shmaddr = (char *) shmat( image->shminfo.shmid, 0, 0 ) ;
    if( image->shminfo.shmaddr == (char *) -1 )
    {
        perror( "Error" ) ;
        return false ;
    }

    image->data = (unsigned int*) image->shminfo.shmaddr ;
    image->shminfo.readOnly = false ;

    // Mark the shared memory segment for removal
    // It will be removed even if this program crashes
    shmctl( image->shminfo.shmid, IPC_RMID, 0 ) ;

    // Allocate the memory needed for the XImage structure
    image->ximage = XShmCreateImage( dsp, XDefaultVisual( dsp, XDefaultScreen( dsp ) ),
                        DefaultDepth( dsp, XDefaultScreen( dsp ) ), ZPixmap, 0,
                        &image->shminfo, 0, 0 ) ;
    if( !image->ximage )
    {
        destroyimage( dsp, image ) ;
        printf( "Error" ": could not allocate the XImage structure\n" ) ;
        return false ;
    }

    image->ximage->data = (char *)image->data ;
    image->ximage->width = width ;
    image->ximage->height = height ;

    // Ask the X server to attach the shared memory segment and sync
    XShmAttach( dsp, &image->shminfo ) ;
    XSync( dsp, false ) ;
    return true ;
}

/* todo: fix! we need to send 24bpp over the network - currently it sends BGRA we only need BGR. */
void getrootwindow( Display * dsp, struct shmimage * image ,int screen_no,int screen_width)
{
    if (screen_no == 0){
        XShmGetImage( dsp, XDefaultRootWindow( dsp ), image->ximage, 0, 0, AllPlanes ) ;
    }
    else{
        XShmGetImage( dsp, XDefaultRootWindow( dsp ), image->ximage, screen_width * screen_no, 0, AllPlanes ) ;
    }
}

int get_frame( struct shmimage * src, struct shmimage * dst )
{
    int sw = src->ximage->width ;
    int sh = src->ximage->height ;
    int dw = dst->ximage->width ;
    int dh = dst->ximage->height ;

    unsigned int * d = dst->data;
    int j, i ;
    for( j = 0 ; j < dh ; ++j )
    {
        for( i = 0 ; i < dw ; ++i )
        {
            int x = (float)(i * src->ximage->width) / (float)dw ;
            int y = (float)(j * src->ximage->height) / (float)dh ;
            *d++ =  src->data[ y * src->ximage->width + x ] ;
        }
    }
    return true ;
}
