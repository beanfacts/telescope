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
#include <sys/socket.h> 

#include <sys/time.h>
#include <rdma/rsocket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h> 

#include "screencap.h"
#include "screencap.c"


#define MAX 90
#define NAME   "Screen Capture"
#define SA struct sockaddr

typedef struct 
{
    Display *dsp;
    struct shmimage *src;
    struct shmimage *dst;
    int screen_number;
    int width;
    int connfd;
    uint16_t port;


} display_args; 

typedef struct {
    int controlPktType;     // 0 -> Mouse Move, 1 -> Mouse Click, 2 -> Keyboard
    
    int mmove_dx;           // Mouse movement x (px)
    int mmove_dy;           // Mouse movement y (px)
    
    int press_keycode;      // Keycode
    int press_state;        // State; 0 -> released, 1 -> pressed

} ControlPacket;

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
ControlPacket decode_packet(char *stream, int size) {

    ControlPacket pkt;

    pkt.controlPktType = -1;
    
    if (size == 9 || stream[0] == 0) 
    {
        // Detect mouse movement
        pkt.controlPktType = (int) stream[0];
        pkt.mmove_dx = *(int *) (&stream[1]);
        pkt.mmove_dy = *(int *) (&stream[5]);
	
	printf("decode -> ");

	for (int x = 0; x < 9; x++) {
		printf("(%d) ", (unsigned char) stream[x]);
	}

	printf("\n");

	printf("decode move: %d, %d\n", pkt.mmove_dx, pkt.mmove_dy);
	if (pkt.mmove_dx > 1000 || pkt.mmove_dx < -1000) { pkt.mmove_dx = 0; }
        if (pkt.mmove_dy > 1000 || pkt.mmove_dy < -1000) { pkt.mmove_dy = 0; }
	
    } 
    else if (size == 6 && (stream[0] == 1 || stream[0] == 2)) 
    {
        // Detect keyboard or mouse button press
        pkt.controlPktType = (int) stream[0];
        pkt.press_keycode = *(int *) (&stream[1]);
        pkt.press_state = (int) stream[5];
    }


    return pkt;
}

bool process_input(ControlPacket pkt, Display *disp, Window xwin) {
    
    bool r = false;

    switch (pkt.controlPktType) {
        case 0:
            XWarpPointer(disp, xwin, 0, 0, 0, 0, 0, pkt.mmove_dx, pkt.mmove_dy);
            printf("move mouse: %d, %d\n", pkt.mmove_dx, pkt.mmove_dy);
            r = true;
            break;
        case 1:
            XTestFakeButtonEvent(disp, pkt.press_keycode, pkt.press_state, 0);
            printf("press mouse: %d, %d\n", pkt.press_keycode, pkt.press_state);
            r = true;
            break;
        case 2:
            XTestFakeKeyEvent(disp, pkt.press_keycode, pkt.press_state, 0);
            printf("press keyboard: %d, %d\n", pkt.press_keycode, pkt.press_state);
            r = true;
            break;
    }

    if (r) {
        XFlush(disp);
    }
    return r;

}


void *func(void* args) 
{ 
    uint16_t port_number = *(uint16_t *) args + 1;
    int sockfd2, connfd2, len2; 
    struct sockaddr_in servaddr2, cli2; 

    sockfd2 = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd2 == -1) { 
        printf("TCP failed to create socket\n"); 
        exit(0); 
    } 
    else
        printf("TCP socket created successfully\n"); 
    memset(&servaddr2, 0, sizeof(servaddr2)); 

    servaddr2.sin_family = AF_INET; 
    servaddr2.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr2.sin_port = htons(port_number); 

    if ((bind(sockfd2, (SA*)&servaddr2, sizeof(servaddr2))) != 0) { 
        printf("TCP socket bind failed\n"); 
        exit(0); 
    } 
    else
        printf("TCP socket successfully binded\n"); 

    if ((listen(sockfd2, 5)) != 0) { 
        printf("TCP socket listen failed\n"); 
        exit(0); 
    } 
    else
        printf("TCP server listening\n");
    len2 = sizeof(cli2);

    connfd2 = accept(sockfd2, (SA*)&cli2, &len2);
    if (connfd2 < 0) {
        printf("TCP failed to accept\n");
        exit(0);
    }
    else
        printf("TCP client accepted\n");
    
    /* Open X11 display */
    Display *Xdisp;
    Window Xrwin;

    Xdisp = XOpenDisplay(0);
    Xrwin = XRootWindow(Xdisp, 0);

	char *buff = calloc(MAX, sizeof(char));

    int n; 
    int size_b;

    while (1) { 
        memset(buff,0 ,MAX); 
        // read the message from client and copy it in buffer 
        read(connfd2, buff, 16); 
        if(buff[0] == 1 | buff[0] == 2) {
            size_b = 6;
        }
        else if (buff[0] == 0)
        {
            size_b = 9;
        }
        ControlPacket pkt = decode_packet(buff, size_b);
        process_input(pkt, Xdisp, Xrwin);

        for(int i=0; i < size_b; i++){
            printf("%hhx", *(buff+i));
        }
        printf("\n");
        memset(buff, 0,MAX); 
        n = 0; 
    }
} 



void* run(void* args)
{
    display_args dsps = *(display_args *) args;
   
    int sockfd, connfd, len;
    int arr[2];
    //uint16_t listen_port = 6969;
    struct sockaddr_in servaddr, cli;

    /* RDMA socket creation */
    sockfd = rsocket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "   could not create rsocket.\n");
        exit(1);
    }
    else
        printf("    rsocket successfully created..\n");
    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(dsps.port);
    if ((rbind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        fprintf(stderr, "   could not bind rsocket.\n");
        exit(1);
    }
    else
        printf("    successfully bound rsocket.\n");
    if ((rlisten(sockfd, 5)) != 0) {
        fprintf(stderr, "   Failed to listen on rsocket.\n");
        exit(1);
    }
    else{
        printf("    listening for clients...\n");
    }
    len = sizeof(cli);
    connfd = raccept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        fprintf(stderr, "   could not accept client.\n");
        exit(1);
    }
    else{
        printf("    accepted client\n");
    }
    /* RDMA socket creation completes*/

    XEvent xevent ;
    int running = true ;
    int initialized = true ;
    int dstwidth = dsps.dst->ximage->width ;
    int dstheight = dsps.dst->ximage->height ;
    int fd = ConnectionNumber(dsps.dsp) ;

    while(running)
    {
        printf("...\n");
        if(initialized)
        {
            printf("getting root...\n");
            getrootwindow(dsps.dsp, dsps.src, dsps.screen_number, dsps.width);
            printf("getting frame...\n");
            if(!get_frame(dsps.src, dsps.dst))
            {
                printf("false");
                return false;
            }
            int size = (dsps.dst->ximage->height)*(dsps.dst->ximage->width)*((dsps.dst->ximage->bits_per_pixel)/8);
            printf("writing...\n");
            rwrite(connfd,dsps.dst->ximage->data,size);
            printf("waiting for vsync\n");
            XSync(dsps.dsp, False);
        }
    }
}

int main(int argc, char* argv[])
{
    //Declare thread id1, id2
    pthread_t id1;
    pthread_t id2;
    Status mt_status;
    mt_status = XInitThreads();
    if (!mt_status){
        fprintf(stderr, "error xx\n");
    }
    else{
        printf("Xint() running (%d) \n", mt_status);
    }

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
    // uint16_t listen_port = 6969;
    uint16_t listen_port;

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

    printf("Capturing X Screen %d @ %d x %d \n", screen_no, width, height);

    /* Allocate the resources required for the X image */
    if(!createimage(dsp, &src, width, height))
    {
        fprintf(stderr, "Failed to create an X image.\n");
        XCloseDisplay(dsp);
        return 1;
    }
    initimage(&dst);
    int dstwidth = width ;
    int dstheight = height ;

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
    display_args argument_for_display = {   .dsp = dsp, 
                                            .src = &src, 
                                            .dst = &dst, 
                                            .screen_number=screen_no, 
                                            .width = width,
                                            .port = listen_port
    };
    
    pthread_create(&id1,NULL, &run, (void *) &argument_for_display);
    pthread_create(&id2,NULL, &func, (void *) &listen_port);

    pthread_join(id1,NULL);
    pthread_join(id2,NULL);

    destroyimage(dsp, &src);
    destroyimage(dsp, &dst);

    XCloseDisplay(dsp);
    return 0;
}
