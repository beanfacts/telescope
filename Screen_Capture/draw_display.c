#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>  // for strtol
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "screencap.h"
#include "screencap.c"
#include "screen_struc.h"
#include <time.h>
#ifdef __linux__
    #include <sys/time.h>
#endif

#define NAME   "Screen Capture"
#define BENCHMARK 1

Window createwindow( Display * dsp, int width, int height )
{
    unsigned long mask = CWBackingStore ;
    XSetWindowAttributes attributes ;
    attributes.backing_store = NotUseful ;
    mask |= CWBackingStore ;
    Window window = XCreateWindow( dsp, DefaultRootWindow( dsp ),
            0, 0, width, height, 0,
            DefaultDepth( dsp, XDefaultScreen( dsp ) ),
            InputOutput, CopyFromParent, mask, &attributes ) ;
    XStoreName( dsp, window, NAME );
    XSelectInput( dsp, window, StructureNotifyMask ) ;
    XMapWindow( dsp, window );
    return window ;
}

void destroywindow( Display * dsp, Window window )
{
    XDestroyWindow( dsp, window );
}

int run( Display * dsp, Window window, struct shmimage * src, struct shmimage * dst , int screen_no, int screen_width)
{
    XGCValues xgcvalues ;
    xgcvalues.graphics_exposures = False ;
    GC gc = XCreateGC( dsp, window, GCGraphicsExposures, &xgcvalues ) ;

    Atom delete_atom = XInternAtom( dsp, "WM_DELETE_WINDOW", False ) ;
    XSetWMProtocols( dsp, window, &delete_atom, True ) ;

    XEvent xevent ;
    int running = true ;
    int initialized = false ;
    int dstwidth = dst->ximage->width ;
    int dstheight = dst->ximage->height ;
    long frames = 0 ;
    int fd = ConnectionNumber( dsp ) ;

    clock_t start, end;
    while( running )
    {
        while( XPending( dsp ) )
        {
            XNextEvent( dsp, &xevent ) ;
            if( ( xevent.type == ClientMessage && xevent.xclient.data.l[0] == delete_atom )
                || xevent.type == DestroyNotify )
            {
                running = false ;
                break ;
            }
            else if( xevent.type == ConfigureNotify )
            {
                if( xevent.xconfigure.width == dstwidth
                    && xevent.xconfigure.height == dstheight )
                {
                    initialized = true ;
                }
            }
        }
        if( initialized )
        {
            start = clock();
            getrootwindow( dsp, src ,screen_no, screen_width) ;
            if( !get_frame( src, dst ) )
            {
                return false ;
            }
            XShmPutImage( dsp, window, gc, dst->ximage, 0, 0, 0, 0, dstwidth, dstheight, False ) ;
            XSync( dsp, False ) ;
            end = clock();
        }
        if (BENCHMARK){
        clock_t spf = (double) (end-start) ;// time per 1/1000000 of second

        //printf("time to get 1 frame: %ld microsecond\n",spf);
        clock_t fps = 1 / (spf * (1e-6));
        printf("FPS:%ld\n",fps);

        }
    }
    return true ;
}

int main( int argc, char * argv[] )
{
    Display * dsp = XOpenDisplay(NULL) ;//create connection to the X server
    int screen = XDefaultScreen( dsp ) ;
    if( !dsp )
    {
        printf( NAME ": could not open a connection to the X server\n" ) ;
        return 1 ;
    }

    if( !XShmQueryExtension( dsp ) )// check the avalibility of the SHM
    {
        XCloseDisplay( dsp ) ;
        printf( NAME ": the X server does not support the XSHM extension\n" ) ;
        return 1 ;
    }
    struct shmimage src, dst ;
    initimage( &src ) ;
    int width, height, screen_no;
    if (argc == 1){
        // if argc is 1 means we have no other modifications and we are running with 1 display
        width = XDisplayWidth( dsp, screen ) ;
        height = XDisplayHeight( dsp, screen ) ;
        screen_no = 0;
    }
    else if (argc == 4 ){
        // if we have more than 1 argc means that we are runing something else
        // usage example ./draw_display 1920 1080 0
        // 1080p display 1st screen
        // ultra-wide can use ./draw_display 3840 1080 0
        width = atoi(argv[1]) ;
        height = atoi(argv[2]);
        screen_no = atoi(argv[3]);
    }

    printf("Resolution: %d X %d \nScreen:%d \n",width,height,screen_no);

    if( !createimage( dsp, &src, width, height ) )
    {
        XCloseDisplay( dsp ) ;
        return 1 ;
    }
    initimage( &dst ) ;
    int dstwidth = width / 2;
    int dstheight = height / 2 ;
    if( !createimage( dsp, &dst, dstwidth, dstheight ) )
    {
        destroyimage( dsp, &src ) ;
        XCloseDisplay( dsp ) ;
        return 1 ;
    }

    if( dst.ximage->bits_per_pixel != 32 )//monitor is 24 + 8 for rendering
    {
        destroyimage( dsp, &src ) ;
        destroyimage( dsp, &dst ) ;
        XCloseDisplay( dsp ) ;
        printf( NAME   ": bits_per_pixel is not 32 bits\n" ) ;
        return 1 ;
    }

    Window window = createwindow( dsp, dstwidth, dstheight ) ;
    run( dsp, window, &src, &dst , screen_no, width) ;
    destroywindow( dsp, window ) ;  

    destroyimage( dsp, &src ) ;
    destroyimage( dsp, &dst ) ;
    XCloseDisplay( dsp ) ;
    return 0 ;
}