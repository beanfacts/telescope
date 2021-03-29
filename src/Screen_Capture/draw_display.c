/* Telescope Server */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/time.h>
#include <rdma/rsocket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "screencap.h"
#include "screencap.c"

#define NAME   "Screen Capture"
#define FRAME  16667
#define PERIOD 1000000
#define NAME   "Screen Capture"
#define SA struct sockaddr

/* todo: fix! ugly function space!!!! */

long timestamp( )
{
    struct timeval tv ;

    gettimeofday( &tv, NULL ) ;
    return tv.tv_sec*1000000L + tv.tv_usec ;
}

int *sock_create(int arr[], uint16_t listen_port)
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;
    sockfd = rsocket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Could not create rsocket.\n");
        exit(1);
    }
    else
        printf("rSocket successfully created..\n");
    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(listen_port);
    if ((rbind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("Could not bind rsocket.\n");
        exit(1);
    }
    else
        printf("Successfully bound rsocket.\n");
    if ((rlisten(sockfd, 5)) != 0) {
        printf("Failed to listen on rsocket.\n");
        exit(1);
    }
    else
        printf("Listening for clients...\n");
    len = sizeof(cli);
    connfd = raccept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("Could not accept client.\n");
        exit(1);
    }
    else
        printf("Accepted client\n");
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

void destroywindow(Display * dsp, Window window)
{
    XDestroyWindow(dsp, window);
}

int run(Display* dsp, struct shmimage* src, struct shmimage* dst, int screen_no, int screen_width, int connfd)
{

    XEvent xevent ;
    int running = true ;
    int initialized = true ;
    int dstwidth = dst->ximage->width ;
    int dstheight = dst->ximage->height ;
    int fd = ConnectionNumber(dsp) ;
    
    while(running)
    {
        printf("...\n");
        if(initialized)
        {
            printf("getting root...\n");
            getrootwindow(dsp, src, screen_no, screen_width);
            printf("getting frame...\n");
            if(!get_frame(src, dst))
            {
                return false;
            }
            int size = (dst->ximage->height)*(dst->ximage->width)*((dst->ximage->bits_per_pixel)/8);
            printf("writing...\n");
            rwrite(connfd,dst->ximage->data,size);
            printf("waiting for vsync\n");
            XSync(dsp, False);
        }
    }
    return true;
}

int main(int argc, char* argv[])
{

    /* Create X server connection */
    Display* dsp = XOpenDisplay(NULL) ;
    int screen = XDefaultScreen(dsp) ;
    
    if(!dsp)
    {
        fprintf(stderr, "Could not open a connection to the X server\n");
        return 1;
    }

    /* Check the X server supports shared memory extensions */
    if(!XShmQueryExtension(dsp))
    {
        XCloseDisplay(dsp);
        fprintf(stderr, "The X server does not support the XSHM extension\n");
        return 1;
    }

    struct shmimage src, dst;
    initimage(&src);
    int width, height, screen_no;
    uint16_t listen_port = 6969;
    
    /*  If no extra arguments are provided, the program will capture the entire
        screen, even if there are multiple monitors connected.
        If the port argument is provided, the port will be set to the default
        listen port, which is 6969.
    */
    if (argc <= 2)
    {
        width = XDisplayWidth(dsp, screen);
        height = XDisplayHeight(dsp, screen);
        screen_no = 0;
        if (argc == 2 && strcmp(argv[1], "0") != 0) {
            listen_port = atoi(argv[1]);
        }
    }
    /*  If multiple arguments are provided, we assume that the user wants to
        capture a single screen of a multi monitor setup. The width, height,
        and index of the screen is provided as follows (in this example
        we want to capture a 4K screen on the left side, listening on the
        default port.

        ./draw_display 0 3840 2160 0
    */
    else if (argc == 4)
    {
        listen_port = (uint16_t) atoi(argv[1]);
        width = atoi(argv[2]) ;
        height = atoi(argv[3]);
        screen_no = atoi(argv[4]);
    }
    else
    {
        fprintf(stderr, "Invalid number of arguments provided.\n");
    }

    int arr[2];
    printf("Capturing X Screen %d @ %d x %d \n", screen_no, width, height);

    sock_create(arr, listen_port);
    
    /* Allocate the resources required for the X image */
    if(!createimage(dsp, &src, width, height))
    {
        fprintf(stderr, "Failed to create an X image.\n");
        XCloseDisplay(dsp);
        return 1;
    }

    initimage(&dst);
    int dstwidth = width / 2;
    int dstheight = height / 2;
    
    if(!createimage(dsp, &dst, dstwidth, dstheight))
    {
        destroyimage(dsp, &src);
        XCloseDisplay(dsp);
        return 1;
    }

    printf("Created XImage successfully.\n");

    /* todo: fix! we need to send 24bpp over the network */
    if( dst.ximage->bits_per_pixel != 32 )
    {
        destroyimage(dsp, &src);
        destroyimage(dsp, &dst);
        XCloseDisplay(dsp);
        printf( NAME   ": bits_per_pixel is not 32 bits\n" );
        return 1;
    }

    printf("Starting event loop...\n");
    run(dsp, &src, &dst , screen_no, width, arr[0]);

    printf("Event loop complete, closing...\n");
    destroyimage(dsp, &src);
    destroyimage(dsp, &dst);

    /* todo: fix! we want to keep the server running */
    rclose(arr[1]);
    XCloseDisplay(dsp);
    return 0;
}