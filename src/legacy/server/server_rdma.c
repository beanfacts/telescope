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
#include <X11/extensions/Xrandr.h>
#include <sys/socket.h> 

#include <sys/time.h>
#include <getopt.h>

#include <signal.h>
#include <sys/wait.h>

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

typedef struct {
    struct sockaddr_in  *clientid;
    struct rdma_cm_id   *connid;
    T_Screen            *screen;
} TRunArgs;

void destroywindow(Display * dsp, Window window)
{
    XDestroyWindow(dsp, window);
}

void *run(void *argptr)
{

    TRunArgs args = *(TRunArgs *) argptr;
    free(argptr);

    // Perform initial data exchange. The client and server need to agree on
    // the resolution and refresh rate, as well as share the RDMA keys for 
    // non-signaled RDMA operations (read/write)
    
    int ret;
    void *receive_buffer, *send_buffer;
    ret = posix_memalign(&receive_buffer, sysconf(_SC_PAGE_SIZE), 1024);
    if (ret || !receive_buffer) { goto buf_error; }
    ret = posix_memalign(&send_buffer, sysconf(_SC_PAGE_SIZE), 1024);
    if (ret || !receive_buffer) { goto buf_error; }

    struct ibv_mr *recv_mr  = rdma_reg_msgs(args.connid, receive_buffer, 1024);
    struct ibv_mr *send_mr  = rdma_reg_msgs(args.connid, send_buffer, 1024);
    struct ibv_wc rcv_wc;
    struct ibv_wc send_wc;

    // Send the server hello message

    T_ServerHello *hello = send_buffer;    
    hello->avail_bw     = -1;
    hello->avail_mem    = 1048576 * 256;
    hello->s_width      = args.screen->width;
    hello->s_height     = args.screen->height;
    hello->s_fps        = args.screen->fps;
    hello->s_format     = T_PF_ARGB_LE_32;
    
    ret = rdma_post_send(args.connid, NULL, send_buffer, sizeof(T_ServerHello), send_mr, NULL);
    if (ret < 0)
    {
        fprintf(stderr, "Could not post send request: %s\n", strerror(errno));
        goto cleanup_rdma_error;
    }

    while ((ret = rdma_get_send_comp(args.connid, &send_wc)) == 0);
    if (ret < 0)
    {
        fprintf(stderr, "Could not check send completion: %s\n", strerror(errno));
        goto cleanup_rdma_error;
    }

    // Wait for Client Hello
    
    ret = rdma_post_recv(args.connid, NULL, receive_buffer, sizeof(T_ClientHello), recv_mr);
    if (ret < 0)
    {
        fprintf(stderr, "Could not post receive request: %s\n", strerror(errno));
        goto cleanup_rdma_error;
    }

    while ((ret = rdma_get_recv_comp(args.connid, &rcv_wc)) == 0);
    if (ret < 0)
    {
        fprintf(stderr, "Error receiving data from client: %s\n", strerror(errno));
        goto cleanup_rdma_error;
    }

    T_ClientHello *client_hello = (T_ClientHello *) receive_buffer;

    if (client_hello->s_width == args.screen->width 
        && client_hello->s_height == args.screen->height 
        && client_hello->s_bpp == args.screen->bpp
        && client_hello->s_format == args.screen->format)
    {
        printf("Server will use its native format.\n");
    }
    else
    {
        printf( "Server will perform image conversion\n"
                "+---------------+--------+--------+\n"
                "| Parameter     | Client | Server |\n"
                "+---------------+--------+--------+\n"
                "| Bits / Pixel  | %6d | %6d |\n"
                "| Screen Width  | %6d | %6d |\n"
                "| Screen Height | %6d | %6d |\n"
                "| Chroma Code   | %6d | %6d |\n"
                "+---------------+--------+--------+\n",
                client_hello->s_bpp, args.screen->bpp,
                client_hello->s_width, args.screen->width,
                client_hello->s_height, args.screen->height,
                client_hello->s_format, args.screen->format
        );
        fprintf(stderr, "Image conversion is currently not supported.\n");
        goto cleanup_rdma_error;
    }

    // Create two image buffers
    if (args.screen->capture_type == CM_X11)
    {
        
        T_ImageList *imlist;
        ret = createimage(args.screen->display, imlist, args.screen->width, args.screen->height, args.screen->bpp, 2);
        if (!ret)
        {
            fprintf(stderr, "Error creating X Image.\n");
        }

    }

    int image_bytes = args.screen->width * args.screen->height * (args.screen->bpp / 8);
    unsigned int *buffer = calloc(1, image_bytes);
    void *confirm_buffer = calloc(1, INLINE_BUFSIZE);
    
    struct ibv_mr *image_mr, *confirm_mr;
    struct ibv_wc wc; 

    confirm_mr = rdma_reg_msgs(args.connid, confirm_buffer, INLINE_BUFSIZE);
    if (!confirm_mr)
    {
        fprintf(stderr, "FATAL: Error registering memory region: %s", strerror(errno));
        return;
    }
    
    image_mr = ibv_reg_mr(args.connid->pd, (void *) buffer, image_bytes, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!image_mr)
    {
        fprintf(stderr, "Error registering memory region: %s", strerror(errno));
    }

    while (1)
    {
        
        printf("Getting image...\n");
        getrootwindow(dsps.dsp, dsps.src, 0);
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
        unlock:
            sem_post(dsps.semaphore);
        // bright: lock
    }

    return;

    buf_error:
        fprintf(stderr, "Buffer allocation failed.\n");
        rdma_disconnect(args.connid);
        if (ret != 0) { fprintf(stderr, "Error disconnecting client\n"); }
        free(receive_buffer);
        free(send_buffer);
        free(args.clientid);
        return;

    cleanup_rdma_error:
        rdma_disconnect(args.connid);
        free(receive_buffer);
        free(send_buffer);
        free(args.clientid);
        return;

}

int main(int argc, char* argv[])
{

    int option_index = 0;
    int ok_flag = 0;

    struct in_addr host;
    
    char *rdma_host     = calloc(1, 256);
    char *rdma_port     = calloc(1, 16);
    int cap_method      = 0;
    
    // Process command line arguments.
    static struct option long_options[] =
    {
        {"host",        required_argument,  0,  'h'},
        {"port",        required_argument,  0,  'p'},
        {"capture",     required_argument,  0,  'c'},
        {0, 0, 0, 0}
    };

    while (1)
    {
        int c = getopt_long_only(argc, argv, "h:p:c:", long_options, &option_index);
        printf("[%s]\n", optarg);

        if (c == -1)
            break;

        switch (c)
        {  
            case 'h':
                strncpy(rdma_host, optarg, 255);
                ok_flag = ok_flag | 0b1;
                break;
            case 'p':
                strncpy(rdma_port, optarg, 15);
                ok_flag = ok_flag | 0b10;
                break;
            case 'c':
                if (strncasecmp(cap_method, "x11", 3) == 0)
                {
                    printf("Capture method: X11\n");
                    cap_method = CM_X11;
                }
                ok_flag = ok_flag | 0b100;
                break;
            default:
                printf("Invalid argument passed\n");
        }
    }

    if (ok_flag != 0b111)
    {
        fprintf(stderr, "Required arguments are invalid or missing.\n");
        return 1;
    }

    T_Screen *screen = calloc(1, sizeof(T_Screen));
    int width, height;
    int ret;

    screen->capture_type = cap_method;
    if (screen->capture_type == CM_X11)
    {
        // Prepare X server connection for multithreading
        ret = XInitThreads();
        if (!ret){
            fprintf(stderr, "X11 threading error: %d\n", ret);
            return 1;
        }
        else
        {
            printf("X11 MT ready (%d) \n", ret);
        }

        // Grab the root display
        screen->display = XOpenDisplay(NULL);
        if(!screen->display)
        {
            fprintf(stderr, "Could not open a connection to the X server\n");
            return 1;
        }

        screen->window = XDefaultRootWindow(screen->display);
        screen->screen_no = (screen->display);

        // Check the X server supports shared memory extensions
        if(!XShmQueryExtension(screen->display))
        {
            XCloseDisplay(screen->display);
            free(screen);
            fprintf(stderr, "Your X server does not support shared memory extensions.\n");
            return 1;
        }

        screen->width   = XDisplayWidth(screen->display, screen->screen_no);
        screen->height  = XDisplayHeight(screen->display, screen->screen_no);
        
        XRRScreenConfiguration *xrr = XRRGetScreenInfo(screen->display, screen->window);
        screen->fps = (uint32_t) XRRConfigCurrentRate(xrr);

        printf("Capturing X Screen %d @ %d x %d \n", screen->screen_no, screen->width, screen->height);

        /*leave image creation for later
        // Allocate the resources required for the X image
        if (!createimage(dsp, imlist, width, height, 32, 2))
        {
            int req_mem = paligned_fsize(width, height, 32);
            fprintf(stderr, 
                "Failed to create X image "
                "(do you have at least %d MB available?)\n", req_mem);
            XCloseDisplay(dsp);
            return 1;
        }
        if (src.ximage->bits_per_pixel != 32)
        {
            destroyimage(dsp, &src);
            XCloseDisplay(dsp);
            printf("Only 32-bit depth is supported (requested %d bits)", src.ximage->bits_per_pixel);
            return 1;
        }
        printf("Created X Image successfully.\n");
        */
    }
    

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

    // Create an RC ("TCP") RDMA channel
    struct rdma_cm_id *sockid;
    new(struct ibv_qp_init_attr, init_attr);
    init_attr.cap.max_send_wr       = TOTAL_WR;
    init_attr.cap.max_recv_wr       = TOTAL_WR;
	init_attr.cap.max_send_sge      = TOTAL_SGE;
    init_attr.cap.max_recv_sge      = TOTAL_SGE;
	init_attr.cap.max_inline_data   = INLINE_BUFSIZE;
	init_attr.sq_sig_all            = 1;
    init_attr.qp_type               = IBV_QPT_RC;
    
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
        return 1;
    }
    printf("Listening on RDMA channel. [%p]\n", (void *) sockid);

    while (1)
    {
        
        // Allocate memory and wait for a new client to connect to the server
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

        // Query the queue pair created between the client and server to
        // determine whether the connection can be used by Telescope

        new(struct ibv_qp_init_attr, conn_attr);
        new(struct ibv_qp_attr, conn_qp_attr);
        ret = ibv_query_qp(connid->qp, &conn_qp_attr, IBV_QP_CAP, &conn_attr);
        if (ret)
        {
            fprintf(stderr, "Could not get QP information for client connection.\n");
            rdma_destroy_ep(connid);
            continue;
        }
        printf("Received QP information\n");
        if (conn_attr.qp_type != IBV_QPT_RC)
        {
            fprintf(stderr, 
                "===========================================================\n"
                "The client connection does not support one or more features:\n"
                "QP Type:         %4d (req. %4d - IBV_QPT_RC)\n"
                "===========================================================\n",
                conn_attr.cap.max_inline_data, INLINE_BUFSIZE,
                conn_attr.qp_type, IBV_QPT_RC
            );
            rdma_destroy_ep(connid);
            continue;
        }
        else
        {
            fprintf(stdout, 
                "===========================================================\n"
                "Incoming connection request feature check passed.\n"
                "QP Type:         %4d (req. %4d - IBV_QPT_RC)\n"
                "===========================================================\n",
                conn_attr.cap.max_inline_data, INLINE_BUFSIZE,
                conn_attr.qp_type, IBV_QPT_RC
            );
            printf("Preparing to accept client\n");
        }

        // Accept the connection if checks pass, otherwise free all the memory
        ret = rdma_accept(connid, NULL);
        if (ret)
        {
            fprintf(stderr, "Could not accept connection");
            rdma_destroy_ep(connid);
            free(client_id);
            continue;
        }

        // Start new thread to service the client
        
        TRunArgs *tra = malloc(sizeof(TRunArgs));
        
        tra->clientid = client_id;
        tra->connid = connid;
        tra->screen = screen;
        
        pthread_t tid;
        pthread_create(&tid, NULL, run, (void *) tra);
}
