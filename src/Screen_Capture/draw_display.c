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
#ifdef __linux__
#include <sys/time.h>
#endif
#include <rdma/rsocket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define NAME   "Screen Capture"
#define FRAME  16667
#define PERIOD 1000000
#define NAME   "Screen Capture"
#define SA struct sockaddr
#define PORT 6969

long timestamp( )
{
    struct timeval tv ;

    gettimeofday( &tv, NULL ) ;
    return tv.tv_sec*1000000L + tv.tv_usec ;
}
int *sock_create(int arr[])
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;
    sockfd = rsocket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("rsocket creation failed...\n");
        exit(0);
    }
    else
        printf("rSocket successfully created..\n");
    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    if ((rbind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("rsocket bind failed...\n");
        exit(0);
    }
    else
        printf("rSocket successfully binded..\n");
    if ((rlisten(sockfd, 5)) != 0) {
        printf("rListen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);
    connfd = raccept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("server acccept failed...\n");
        exit(0);
    }
    else
        printf("server acccept the client...\n");
    arr[0] = connfd;
    arr[1] = sockfd;
    // func(connfd);
    return arr;
}

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

int run( Display * dsp, struct shmimage * src, struct shmimage * dst , int screen_no, int screen_width, int connfd)
{
    //XGCValues xgcvalues ;
    //xgcvalues.graphics_exposures = False ;
    //GC gc = XCreateGC( dsp, window, GCGraphicsExposures, &xgcvalues ) ;

    //Atom delete_atom = XInternAtom( dsp, "WM_DELETE_WINDOW", False ) ;
    //XSetWMProtocols( dsp, window, &delete_atom, True ) ;

    XEvent xevent ;
    int running = true ;
    int initialized = true ;
    int dstwidth = dst->ximage->width ;
    int dstheight = dst->ximage->height ;
    int fd = ConnectionNumber( dsp ) ;
    while( running )
    {

        if( initialized )
        {
            getrootwindow( dsp, src ,screen_no, screen_width) ;
            if( !get_frame( src, dst ) )
            {
                return false ;
            }
            int size = (dst->ximage->height)*(dst->ximage->width)*((dst->ximage->bits_per_pixel)/8);
            rwrite(connfd,dst->ximage->data,size);
            XSync( dsp, False ) ;


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
    int arr[2];
    printf("Resolution: %d X %d \nScreen:%d \n",width,height,screen_no);
    sock_create(arr);
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

    run( dsp, &src, &dst , screen_no, width, arr[0]) ;

    destroyimage( dsp, &src ) ;
    destroyimage( dsp, &dst ) ;
    rclose(arr[1]);
    XCloseDisplay( dsp ) ;
    return 0 ;
}