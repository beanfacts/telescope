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
#include <X11/extensions/XTest.h>
#include <sys/socket.h> 

#include <sys/time.h>

#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <infiniband/verbs.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h> 

#include <semaphore.h>

#include "../common/rdma_common.h"

#include "screencap.h"
#include "screencap.c"

#include <fcntl.h>

#define __T_DEBUG__
#define INLINE_BUFSIZE 32
#define TOTAL_WR 3
#define TOTAL_SGE 1
static struct rdma_cm_id *sockid, *connid;

// Do you want to use TCP or RDMA event notification mechanisms?
static bool use_tcp = true;


typedef struct {
    uint64_t addr;  // Remote memory buffer address
    uint64_t size;  // Remote memory buffer size
    uint32_t nbufs; // Number of buffers available
    uint32_t rkey;  // Remote key to access memory segment
} RemoteMR;


typedef struct
{
    struct rdma_cm_id   *connid;        // RDMA connection identifier
    Display             *dsp;           // X11 display
    Window              win;            // X11 window
    XImage              *ximage;        // X11 image
    void                *rcv_buf;       // Receive buffer
    int                 rcv_bufsize;    // Receive buffer size
    int                 poll_usec;      // Polling delay
    sem_t               *semaphore;     // Semaphore to sync RDMA
    int                 tcp_fd;         // TCP event notification FD
} ControlArgs;


typedef struct 
{
    struct rdma_cm_id   *connid;        // RDMA connection identifier
    RemoteMR            *remote_mr;     // Remote MR information
    Display             *dsp;           // X11 display
    struct shmimage     *src;           // X11 shared memory segment information
    int                 screen_number;  // X11 screen index
    int                 width;          // Screen width
    sem_t               *semaphore;     // Semaphore to sync RDMA
    int                 tcp_fd;         // TCP event notification FD
} display_args;


bool process_input(char *msg, Display *disp, Window xwin) {
    int dx, dy, code, input_type;
    bool state;
    bool r = false;

    input_type = msg[0];
    printf("Input type: %d", msg[0]);

    switch (input_type) {
        case 0: // Mouse move
        {
            dx = *(int *) (msg + 1);
            dy = *(int *) (msg + 5);
            #ifdef __T_DEBUG__ 
            printf("move mouse: %d, %d\n", dx, dy);
            #endif
            XWarpPointer(disp, xwin, 0, 0, 0, 0, 0, dx, dy);
            r = true;
            break;
        }
        case 1: // Mouse click
        {
            code = *(int *) (msg + 1);
            state = (int) *(char *) (msg + 5);
            #ifdef __T_DEBUG__ 
            printf("press mouse: %d, %d\n", code, state);
            #endif
            XTestFakeButtonEvent(disp, code, state, 0);
            r = true;
            break;
        }
        case 2: // Keyboard press
        {
            code = *(int *) (msg + 1);
            state = (int) *(char *) (msg + 5);
            #ifdef __T_DEBUG__
            printf("press keyboard: %d, %d\n", code, state);
            #endif
            XTestFakeKeyEvent(disp, code, state, 0);
            r = true;
            break;
        }
    }
    if (r) 
    {
        XFlush(disp);   // Ensure the command is received immediately
    }
    return r;
}


void destroywindow(Display * dsp, Window window)
{
    XDestroyWindow(dsp, window);
}


void *control(void *void_args)
{
    ControlArgs *args = (ControlArgs *) void_args;
    struct ibv_wc rcv_wc;
    struct ibv_mr *rcv_mr;
    int ret, rcv_type;

    // The caller of this function can specify whether to use a specific
    // receive side buffer, otherwise the thread will allocate its own
    // memory to store received messages. To force the thread to allocate
    // its own memory, set the argument value to NULL.
    if(!(args->rcv_buf))
    {
        args->rcv_buf = calloc(INLINE_BUFSIZE, 1);
        args->rcv_bufsize = INLINE_BUFSIZE;
    }

    // Register receive buffer memory region for signalling data
    rcv_mr = rdma_reg_msgs(args->connid, args->rcv_buf, INLINE_BUFSIZE);
    if (!rcv_mr)
    {
        fprintf(stderr, "Could not register memory region.\n");
    }
     
    printf("Registered buffer\n");

    int speed_test = 0;
    struct timeval t0, t1;
    
    gettimeofday(&t0, NULL);

    // Resize the receive connection queue to handle more incoming data
    // especially from mouse packets
    ret = ibv_resize_cq(args->connid->recv_cq, 10000);
    if (ret)
    {
        fprintf(stderr, "CQ resize failed: %s\n", strerror(errno));
    }
    
    int semvalue;
    
    while (1)
    {
        sem_getvalue(args->semaphore, &semvalue);
        if (semvalue == 0)
        {
            sem_post(args->semaphore);
        }
        else
        {
            printf("Client asked for receive while one is pending\n");
        }
        sleep(16667);
    }

    /*
    while (1)
    {

        // In every loop iteration, the latest pending work request is requested
        // for placement inside a receive buffer. The hardware is informed accordingly.
        ret = rdma_post_recv(args->connid, NULL, args->rcv_buf, INLINE_BUFSIZE, rcv_mr);
        if (ret)
        {
            fprintf(stderr, "Could not post the receive request to the queue: %s\n", strerror(errno));
            //continue;
        }  

        while ((ret = rdma_get_recv_comp(args->connid, &rcv_wc)) == 0)
        {
            printf("Waiting for incoming data...\n");
        }

        if (rcv_wc.status)
        {
            fprintf(stderr, "Error! Status: %d (unrecoverable)\n", rcv_wc.status);
            return NULL;
        }
        
        if (!ret)
        {
            printf("Status of pending CQ is 0\n");
            continue;
        }

        // Determine what to do with the incoming data
        printf("Received incoming data.\n");
        fprintf(stderr, "Message type: %d\n", *(int *) args->rcv_buf);

        rcv_type = *(int *) args->rcv_buf;
        switch (rcv_type)
        {
            case T_MSG_KEEP_ALIVE:
            {
                speed_test++;
                gettimeofday(&t1, NULL);
                if (t1.tv_sec - t0.tv_sec >= 1)
                {
                    printf("Speed: %d msgs/sec\n", speed_test);
                    fflush(stdout);
                    speed_test = 0;
                    gettimeofday(&t0, NULL);
                }
                break;
            }
            case T_INLINE_DATA_HID:
            {
                process_input((char *) (args->rcv_buf) + sizeof(int), args->dsp, args->win);
                break;
            }
            case T_REQ_BUFFER_WRITE:
            {
                sem_getvalue(args->semaphore, &semvalue);
                if (semvalue == 0)
                {
                    sem_post(args->semaphore);
                }
                else
                {
                    printf("Client asked for receive while one is pending\n");
                }
                break;
            }
            default:
            {
                //fprintf(stderr, "Invalid message type: %d\n", rcv_type);
            }
        }
    }
    */
}


void *run(void *args)
{

    display_args dsps = *(display_args *) args;

    XEvent xevent;
    // int dstwidth = dsps.dst->ximage->width;
    // int dstheight = dsps.dst->ximage->height;
    int fd = ConnectionNumber(dsps.dsp);
    int width = 1920;
    int height = 1080;
    int bpp = 32;
    int ret;

    int image_bytes = width * height * (bpp / 8);
    unsigned int *buffer = calloc(1, image_bytes);
    void *confirm_buffer = calloc(1, INLINE_BUFSIZE);
    int random = 0;
    
    struct ibv_mr *image_mr, *confirm_mr;
    struct ibv_wc wc; 

    confirm_mr = rdma_reg_msgs(dsps.connid, confirm_buffer, INLINE_BUFSIZE);
    if (!confirm_mr)
    {
        fprintf(stderr, "Error registering memory region: %s", strerror(errno));
    }

    printf("SHM section: %lu, X11 section: %lu\n", dsps.src->shminfo.shmaddr, dsps.src->ximage->data);
    
    image_mr = ibv_reg_mr(dsps.connid->pd, (void *) dsps.src->ximage->data, image_bytes, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!image_mr)
    {
        fprintf(stderr, "Error registering memory region: %s", strerror(errno));
    }

    while (1)
    {
        // bright: wait for unlock
        sem_wait(dsps.semaphore);
        printf("Getting image...\n");
        // memset(dsps.src->ximage->data, 1, image_bytes);
        // for(int i=0; i < 50; i++)
        // {
        //     printf("%hhx", *((char *) dsps.src->ximage->data + i));
        // }
        // printf("\n");
        getrootwindow(dsps.dsp, dsps.src, 0, 0);
        XSync(dsps.dsp, False);
        printf("Writing data...\n");

        for(int i=0; i < 50; i++)
        {
            printf("%hhx", *((char *) dsps.src->ximage->data + i));
        }
        printf("\n");

        #ifdef __T_DEBUG__
        printf("Posting write request...\n");
        #endif

        *(int *) buffer = random++;
        // memset(dsps.src->ximage->data, 1, image_bytes);
        ret = rdma_post_write(dsps.connid, NULL, (void *) dsps.src->ximage->data, image_bytes, image_mr, IBV_SEND_FENCE, 
            dsps.remote_mr->addr, dsps.remote_mr->rkey);
        if (ret)
        {
            fprintf(stderr, "Could not perform a post write: %s", strerror(errno));
            goto unlock;
        }
        while ((ret = rdma_get_send_comp(dsps.connid, &wc)) == 0)
        {
            printf("Waiting for client to confirm ...\n");
        }
        if (wc.status)
        {
            fprintf(stderr, "WC error (%d) on write #1\n", wc.status);
            goto unlock;
        }
        else
        {
            printf("Wirote data\n");
        }

        #ifdef __T_DEBUG__
        printf("Posting send request...\n");
        #endif

        if (use_tcp)
        {
            write(dsps.tcp_fd, confirm_buffer, INLINE_BUFSIZE);
            if (ret < 0)
            {
                fprintf(stderr, "Could not write any data ... %s\n", strerror(errno));
            }
            goto unlock;
        }
        else
        {
            ret = rdma_post_send(dsps.connid, NULL, confirm_buffer, INLINE_BUFSIZE, confirm_mr, IBV_SEND_INLINE);
            
            while ((ret = rdma_get_send_comp(dsps.connid, &wc)) == 0)
            {
                printf("Waiting for client to confirm my confirmation ...\n");
            }

            if (wc.status)
            {
                fprintf(stderr, "WC error (%d) on write #2\n", wc.status);
                goto unlock;
            }
            #ifdef __T_DEBUG__
            printf("Posting send complete...\n");
            #endif

            goto unlock;
        }
        unlock:
            sem_post(dsps.semaphore);
        // bright: lock
    }

}

int main(int argc, char* argv[])
{

    int ret = XInitThreads();
    if (!ret){
        fprintf(stderr, "X11 threading error: %d\n", ret);
        return 1;
    }
    else
    {
        printf("X11 MT ready (%d) \n", ret);
    }

    // Create X server connection
    Display *dsp = XOpenDisplay(NULL);
    if(!dsp)
    {
        fprintf(stderr, "Could not open a connection to the X server\n");
        return 1;
    }

    Window dsp_root = XDefaultRootWindow(dsp);
    int screen = XDefaultScreen(dsp);

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

    screen_no = 0;
    width = XDisplayWidth(dsp, screen);
    height = XDisplayHeight(dsp, screen);
    listen_port = 6969;

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
    
    /*
    // Initialize the X image for use
    initimage(&dst);
    int dstwidth = width;
    int dstheight = height;

    if(!createimage(dsp, &dst, dstwidth, dstheight, 32))
    {
        destroyimage(dsp, &src);
        XCloseDisplay(dsp);
        return 1;
    }

    //  Telescope currently only supports 32 bits per pixel, the internal rendering
    //  depth of X11. For optimal performance, this mode is used to prevent a secondary
    //  buffer from being necessary to store a subsampled image.
    
    printf("Created X Image successfully.\n");
    if(dst.ximage->bits_per_pixel != 32)
    {
        destroyimage(dsp, &src);
        destroyimage(dsp, &dst);
        XCloseDisplay(dsp);
        printf("Only 32-bit depth is supported (requested %d bits)", dst.ximage->bits_per_pixel);
        return 1;
    }
    */

    // Use rdma_cm to create an RDMA RC connection.
    // Resolve the address into a connection identifier
    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;

    printf("Attempting to listen to %s:%s\n", argv[1], argv[2]);

    // Use a TCP socket for initial connection establishment and, with
    // RAI_PASSIVE indicate the RDMA address info is for the passive 
    // (server) side.
    hints.ai_port_space = RDMA_PS_TCP;
    hints.ai_flags = RAI_PASSIVE;
    ret = rdma_getaddrinfo(argv[1], argv[2], &hints, &host_res);
    if (ret)
    {
        fprintf(stderr, "Could not resolve host.\n");
        return 1;
    }

    new(struct ibv_qp_init_attr, init_attr);
    // Allow the same QP to be used by two threads with 256 bytes
    // allocated for inline data, such as HID input. Also create
    // it as reliable connected (TCP equivalent)
    // init_attr.cap.max_recv_wr = TOTAL_WR;
    // init_attr.cap.max_recv_sge = TOTAL_SGE;
    // init_attr.cap.max_inline_data = INLINE_BUFSIZE;
    //init_attr.qp_type = IBV_QPT_RC;

    init_attr.cap.max_send_wr = TOTAL_WR;
    init_attr.cap.max_recv_wr = TOTAL_WR;
	init_attr.cap.max_send_sge = TOTAL_SGE;
    init_attr.cap.max_recv_sge = TOTAL_SGE;
	init_attr.cap.max_inline_data = INLINE_BUFSIZE;
	init_attr.sq_sig_all = 1;
    
    ret = rdma_create_ep(&sockid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection\n");
        return 1;
    }
    printf("Created RDMA connection\n");

    // Start listening for incoming connections
    ret = rdma_listen(sockid, 0);
    if (ret)
    {
        fprintf(stderr, "Failed to listen on RDMA channel.\n");
    }
    printf("Listening on RDMA channel\n");
    printf("SockID: %d\n",sockid);

    int sockfd, connfd, len; 
    struct sockaddr_in servaddr;

    if (use_tcp)
    {
    
        // Create socket and set up addresses

        sockfd = socket(AF_INET, SOCK_STREAM, 0); 
        if (sockfd == -1)
        { 
            printf("Could not create the socket.\n");
            return 1;
        } 

        memset(&servaddr, 0, sizeof(servaddr)); 
    
        int port = (short) atoi(argv[2]);

        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
        servaddr.sin_port = htons(9999);
    
        // Bind address

        if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0)
        { 
            printf("Cannot bind to socket.\n"); 
            return 1;
        }
        
        if ((listen(sockfd, 5)) != 0) { 
            printf("Cannot listen on socket.\n"); 
            return 1;
        }
        
        fprintf(stdout, 
            "Client info: %s:%d\n", 
            inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    }

    int sa_len = sizeof(struct sockaddr);

    while (1)
    {   
        struct sockaddr_in *client_id = calloc(1, sizeof(struct sockaddr_in));
        struct rdma_cm_id *connid = calloc(1, sizeof(struct rdma_cm_id));
        int tcp_client_fd;
        
        printf("ConnID: %d %d\n", &connid, connid);

        ret = rdma_get_request(sockid, &connid); 
        if (ret)
        {
            fprintf(stderr, "Failed to read connection request.\n");
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
        printf("Received QP information\n");
        
        // Disconnect client if there isn't enough inline buffer space or SGEs.
        if (!(conn_attr.cap.max_inline_data >= INLINE_BUFSIZE
            && conn_attr.qp_type == IBV_QPT_RC))
        {
            fprintf(stderr, 
                "Connection does not support features required by Telescope:\n"
                "Max inline data: %d\t(req. %d)\n"
                "QP Type:         %d\t(req. %d - IBV_QPT_RC)\n",
                conn_attr.cap.max_inline_data, INLINE_BUFSIZE,
                conn_attr.qp_type, IBV_QPT_RC
            );
            rdma_destroy_ep(connid);
            continue;
        }
        printf("Preparing to accept client\n");

        // Accept the connection if the checks pass
        ret = rdma_accept(connid, NULL);
        if (ret)
        {
            fprintf(stderr, "Could not accept connection");
            continue;
        }

        void *recieve_buffer = calloc(1, INLINE_BUFSIZE);
        struct ibv_mr *meta_mr = rdma_reg_msgs(connid, recieve_buffer, INLINE_BUFSIZE);
        struct ibv_wc rcv_wc;

        if (!meta_mr)
        {
            fprintf(stderr, "Cannot register MR to get meta.\n");
        }

        printf("Accepted client. Getting memory information...\n");

        ret = rdma_post_recv(connid, NULL, recieve_buffer, INLINE_BUFSIZE, meta_mr);
        if (ret)
        {
            fprintf(stderr, "Could not post the receive request to the queue: %s\n", strerror(errno));
        }

        while ((ret = rdma_get_recv_comp(connid, &rcv_wc)) == 0)
        {
            printf("Waiting for incoming data...\n");
        }
        
        T_InlineBuff *mem_msg = (T_InlineBuff *) recieve_buffer;

        for(int i=0; i < INLINE_BUFSIZE ; i++)
        {
        printf("%hhx", *((char *) recieve_buffer + i));
        }
        printf("\n");
        
        for(int i=0; i < sizeof(T_InlineBuff) ; i++)
        {
        printf("%hhx", *((char *) mem_msg + i));
        }
        printf("\n");

        printf( "------- Client information -------\n"
                "(Type: %20lu) (Addr: %20lu)\n"
                "(Size: %20lu) (Rkey: %20lu)\n"
                "(Bufs: %20lu)\n" 
                "----------------------------------\n",
        (uint64_t) mem_msg->data_type, (uint64_t) mem_msg->addr, (uint64_t) mem_msg->size,
        (uint64_t) mem_msg->rkey, (uint64_t) mem_msg->numbufs);

        if (use_tcp)
        {
            printf("Got a connection request. Waiting for TCP channel...\n");
            tcp_client_fd = accept(sockfd, (struct sockaddr *) client_id, &sa_len); 
            if (tcp_client_fd < 0)
            { 
                printf("Server could not accept the client.\n");
                continue;
            }
        }
        else
        {
            tcp_client_fd = -1;
            free(client_id);
        }

        pthread_t id1;
        pthread_t id2;
        
        // Create a new semaphore to be shared between the calling threads
        // of the process. I
        sem_t *semaphore = malloc(sizeof(sem_t));
        sem_init(semaphore, 0, 0);

        ControlArgs *args_control = malloc(sizeof(ControlArgs));
        
        args_control->connid = connid;
        args_control->dsp = dsp;
        args_control->ximage = NULL;
        args_control->win = dsp_root;
        args_control->rcv_buf = NULL;
        args_control->rcv_bufsize = INLINE_BUFSIZE;
        args_control->poll_usec = 1000;
        args_control->semaphore = semaphore;
        args_control->tcp_fd = tcp_client_fd;
        
        display_args *args_display = malloc(sizeof(display_args));
        
        args_display->connid = connid;
        args_display->dsp = dsp;
        args_display->screen_number = 0;
        args_display->width = 1920;
        args_display->src = &src;
        args_display->semaphore = semaphore;
        args_display->tcp_fd = tcp_client_fd;

        args_display->remote_mr = malloc(sizeof(RemoteMR));
        args_display->remote_mr->addr = (uint64_t) mem_msg->addr;
        args_display->remote_mr->nbufs = mem_msg->numbufs;
        args_display->remote_mr->rkey = mem_msg->rkey;
        args_display->remote_mr->size = mem_msg->size;

        pthread_create(&id1, NULL, control, (void *) args_control);
        pthread_create(&id2, NULL, run, (void *) args_display);
        
        free(recieve_buffer);
    }
}
