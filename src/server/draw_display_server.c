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
#include "../common/rdma_common.h"
#include "../common/queue_safe.h"

#include <fcntl.h>

#define __T_DEBUG__

typedef struct 
{
    Display             *dsp;
    struct shmimage     *src;
    struct shmimage     *dst;
    int                 screen_number;
    int                 width;
    int                 connfd;
    uint16_t            port;
} display_args;


typedef struct {
    int controlPktType;     // 0 -> Mouse Move, 1 -> Mouse Click, 2 -> Keyboard
    int mmove_dx;           // Mouse movement x (px)
    int mmove_dy;           // Mouse movement y (px)
    int press_keycode;      // Keycode
    int press_state;        // State; 0 -> released, 1 -> pressed
} ControlPacket;

void *rdma_buf;

Window createwindow(Display * dsp, int width, int height)
{
    unsigned long mask = CWBackingStore;
    XSetWindowAttributes attributes;
    attributes.backing_store = NotUseful;
    mask |= CWBackingStore;
    Window window = XCreateWindow(dsp, DefaultRootWindow(dsp),
                                   0, 0, width, height, 0,
                                   DefaultDepth(dsp, XDefaultScreen(dsp)),
                                   InputOutput, CopyFromParent, mask, &attributes);
    XStoreName(dsp, window, NAME);
    XSelectInput(dsp, window, StructureNotifyMask);
    XMapWindow(dsp, window);
    return window;
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
        pkt.controlPktType = (int) stream[0];
        pkt.mmove_dx = *(int *) (&stream[1]);
        pkt.mmove_dy = *(int *) (&stream[5]);

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
        case 0: // Move the mouse
            XWarpPointer(disp, xwin, 0, 0, 0, 0, 0, pkt.mmove_dx, pkt.mmove_dy);
            #ifdef __T_DEBUG__ 
            printf("move mouse: %d, %d\n", pkt.mmove_dx, pkt.mmove_dy);
            #endif
            r = true;
            break;
        case 1: // Simulate a mouse click
            XTestFakeButtonEvent(disp, pkt.press_keycode, pkt.press_state, 0);
            #ifdef __T_DEBUG__ 
            printf("press mouse: %d, %d\n", pkt.press_keycode, pkt.press_state);
            #endif
            r = true;
            break;
        case 2: // Simulate a keyboard keypress
            XTestFakeKeyEvent(disp, pkt.press_keycode, pkt.press_state, 0);
            #ifdef __T_DEBUG__
            printf("press keyboard: %d, %d\n", pkt.press_keycode, pkt.press_state);
            #endif
            r = true;
            break;
    }

    if (r) 
    {
        XFlush(disp);   // Ensure the command is received immediately
    }
    
    return r;

}


void *func(void* args) 
{ 
    uint16_t port_number = *(uint16_t *) args + 1;
    int fd_tcp, connfd2, len2; 
    struct sockaddr_in servaddr2, cli2; 

    /*  Create a TCP socket for transmission of keyboard and mouse data
        over the network
    */
    fd_tcp = socket(AF_INET, SOCK_STREAM, 0); 
    if (fd_tcp == -1) { 
        printf("TCP failed to create socket\n"); 
        exit(0); 
    } 
    else
    {
        printf("TCP socket created successfully\n"); 
    }
        
    memset(&servaddr2, 0, sizeof(servaddr2)); 

    servaddr2.sin_family = AF_INET; 
    servaddr2.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr2.sin_port = htons(port_number);

    if ((bind(fd_tcp, (SA*)&servaddr2, sizeof(servaddr2))) != 0) { 
        printf("TCP socket bind failed\n"); 
        exit(0); 
    } 
    else
        printf("TCP socket successfully binded\n"); 

    if ((listen(fd_tcp, 5)) != 0) { 
        printf("TCP socket listen failed\n"); 
        exit(0); 
    } 
    else
        printf("TCP server listening\n");
    len2 = sizeof(cli2);

    connfd2 = accept(fd_tcp, (SA*)&cli2, &len2);
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
        memset(buff, 0, MAX); 
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

/*  Create an RDMA capable server socket.
    On success, returns the socket file descriptor.
    On failure, returns -1.

    Notes
    - The caller is responsible for accepting clients after the socket has been created.
    - rsocket file descriptors cannot be mixed with standard file descriptors.
int create_rdma_socket(struct sockaddr_in params, bool async) {
   
    int sockfd, connfd;
    int ret;

    sockfd = rsocket(params.sin_family, SOCK_STREAM, 0);
    
    if (async)
    {
        ret = rfcntl(sockfd, F_SETFL, O_NONBLOCK);
    }
    
    if (sockfd == -1)
    {
        fprintf(stderr, "RDMA socket could not be created.\n");
        return -1;
    }
    else
    {
        printf("Created RDMA socket.\n");
    }

    if ((rbind(sockfd, (SA*)&params, sizeof(params))) != 0)
    {
        fprintf(stderr, "Could not bind to %s:%d\n",
            inet_ntoa(params.sin_addr), ntohs(params.sin_port));
        return -1;
    }
    else
    {
        printf("Bound RDMA socket.\n");
    }

    if ((rlisten(sockfd, 10)) != 0)
    {
        fprintf(stderr, "Failed to listen on %s:%d\n",
            inet_ntoa(params.sin_addr), ntohs(params.sin_port));
        return -1;
    }
    else
    {
        printf("Listening on %s:%d\n",
            inet_ntoa(params.sin_addr), ntohs(params.sin_port));
    }

    return connfd;
}
*/

void* run_client(void* args)
{
    display_args dsps = *(display_args *) args;
    XEvent xevent;
    int running = true;
    int initialized = true;
    int dstwidth = dsps.dst->ximage->width;
    int dstheight = dsps.dst->ximage->height;
    int fd = ConnectionNumber(dsps.dsp);

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

/*  Request the client to create a memory region for the
    server to push its data.
int setup_client_resources(int connfd)
{
    printf("Read %lu bytes\n", read_bytes);
    printf("Message type -> %d\n", hdr->msg_type);
    printf("Message length -> %d\n", hdr->length);

    // Receive client's memory buffer data
    if (hdr->msg_type == T_MSG_BUFFER_DATA)
    {
        read_bytes = rread(connfd, mrbuf,sizeof(T_BufferData));
        printf("Read %lu bytes\n", read_bytes);
        printf("Client buffer size %ld bytes, at location %lu\n", mrdata->bufsize, mrdata->offset);
    }

    hdr->length = 0;
    hdr->msg_id = 9999;
    hdr->msg_type = T_MSG_INVALID;
    hdr->num_msgs = 0;

    rwrite(connfd, hdr, sizeof(T_MsgHeader));

    read_bytes = 0;
    while (!read_bytes) {
        read_bytes = riowrite(connfd, frame1_msg, strlen(frame1_msg), mrdata->offset, MSG_DONTWAIT | SOCK_NONBLOCK);
        if (read_bytes > 0) {
                printf("YAY");
            } else {
                perror("riowrite");
            }
        printf("Wrote %lu bytes.\n", read_bytes);
        usleep(1000000);
    }
    
    usleep(1000000);

    hdr->msg_type = T_MSG_SERVER_XFER_DONE;
    hdr->length = sizeof(int);
    hdr->msg_id = rand();

    memcpy(xferbuf, hdr, sizeof(T_MsgHeader));
    *(int *) (xferbuf + sizeof(T_MsgHeader)) = frame_index;

    rwrite(connfd, xferbuf, sizeof(xferbuf));
}
*/

void *run(void *args)
{

}

int main(int argc, char* argv[])
{

    int servfd;
    int ret;
    
    pthread_t id1;
    pthread_t id2;
    Status mt_status;
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    mt_status = XInitThreads();
    if (!mt_status){
        fprintf(stderr, "X11 threading error: %d\n", mt_status);
    }
    else
    {
        printf("X11 MT ready (%d) \n", mt_status);
    }

    // Create X server connection
    Display* dsp = XOpenDisplay(NULL);
    int screen = XDefaultScreen(dsp);
    if(!dsp)
    {
        fprintf(stderr, "Could not open a connection to the X server.\n");
        return 1;
    }

    // Check the X server supports shared memory extensions
    if(!XShmQueryExtension(dsp))
    {
        XCloseDisplay(dsp);
        fprintf(stderr, "Your X server does not support shared memory extensions.\n");
        return 1;
    }

    struct shmimage src, dst;
    initimage(&src);
    int width, height, screen_no;
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
            servaddr.sin_port = htons(atoi(argv[1]));
        }
        else
        {
            servaddr.sin_port = htons(6969);    
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
        width = atoi(argv[2]);
        height = atoi(argv[3]);
        screen_no = atoi(argv[4]);
    }
    else
    {
        fprintf(stderr, "Invalid number of arguments provided.\n");
    }

    printf("Capturing X Screen %d @ %d x %d \n", screen_no, width, height);

    // Allocate the resources required for the X image
    if(!createimage(dsp, &src, width, height, 4))
    {
        int req_mem = (width * height * 4) / 1048576;
        fprintf(stderr, 
            "Failed to create X image "
            "(do you have at least %d MB available?)\n", req_mem);
        XCloseDisplay(dsp);
        return 1;
    }
    
    // Initialize the X image for use
    initimage(&dst);
    int dstwidth = width;
    int dstheight = height;

    if(!createimage(dsp, &dst, dstwidth, dstheight))
    {
        destroyimage(dsp, &src);
        XCloseDisplay(dsp);
        return 1;
    }

    /*  Telescope currently only supports 32 bits per pixel, the internal rendering
        depth of X11. For optimal performance, this mode is used to prevent a secondary
        buffer from being necessary to store a subsampled image.
    */
    printf("Created X Image successfully.\n");
    if(dst.ximage->bits_per_pixel != 32)
    {
        destroyimage(dsp, &src);
        destroyimage(dsp, &dst);
        XCloseDisplay(dsp);
        printf("Only 32-bit depth is supported (requested %d bits)", dst.ximage->bits_per_pixel);
        return 1;
    }

    display_args argument_for_display = {   .dsp = dsp, 
                                            .src = &src, 
                                            .dst = &dst, 
                                            .screen_number=screen_no, 
                                            .width = width,
                                            .port = listen_port
    };

    // Use rdma_cm to create an RDMA RC connection.
    // Resolve the address into a connection identifier
    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;

    hints.ai_port_space = RDMA_PS_TCP;
    ret = rdma_getaddrinfo(argv[1], argv[2], &hints, &host_res);
    if (ret)
    {
        fprintf(stderr, "Could not resolve host.\n");
        return 1;
    }

    new(struct ibv_qp_init_attr, init_attr);
    struct rdma_cm_id *sockid, *connid;
    
    // Allow the same QP to be used by two threads with 256 bytes
    // allocated for inline data, such as HID input. Also create
    // it as reliable connected (TCP equivalent)
    init_attr.cap.max_send_wr = 2;
    init_attr.cap.max_recv_wr = 2;
    init_attr.cap.max_send_sge = 2;
    init_attr.cap.max_recv_sge = 2;
    init_attr.cap.max_inline_data = 256;
    init_attr.qp_type = IBV_QPT_RC;
    ret = rdma_create_ep(&sockid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection\n");
        return 1;
    }

    // Start listening for incoming connections
    ret = rdma_listen(sockid, 5);
    if (ret)
    {
        fprintf(stderr, "Failed to listen on RDMA channel.\n");
    }

    while (1)
    {
        ret = rdma_get_request(sockid, &connid);
        if (ret)
        {
            fprintf(stderr, "Failed to read connection request.\n");
            usleep(10000);
            continue;
        }

        // Determine whether the connection supports the features required
        // for Telescope
        new(struct ibv_qp_init_attr, conn_attr);
        new(struct ibv_qp_attr, conn_qp_attr);
        
        // Disconnect client if QP information cannot be retrieved.
        ret = ibv_query_qp(connid->qp, &conn_qp_attr, IBV_QP_CAP, &conn_attr);
        if (ret)
        {
            fprintf(stderr, "Could not get QP information for client connection.\n");
            rdma_destroy_ep(connid);
            continue;
        }
        
        // Disconnect client if there isn't enough inline buffer space or SGEs.
        if (!(conn_attr.cap.max_inline_data >= 256 && conn_attr.cap.max_send_sge >= 2
            && conn_attr.qp_type == IBV_QPT_RC))
        {
            fprintf(stderr, 
                "Connection does not support features required by Telescope:\n"
                "Max inline data: %d\t(req. %d)\n"
                "QP Type:         %d\t(req. %d)\n"
                "Max send SGEs:   %d\t(req. %d)\n"
            );
            rdma_destroy_ep(connid);
            continue;
        }

        // Accept the connection if the checks pass
        ret = rdma_accept(connid, NULL);
        if (ret)
        {
            fprintf(stderr, "Could not accept connection");
            continue;
        }

        pthread_create()

        // Create a new thread to accept the client connection.

    }
}
