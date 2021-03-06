/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope RDMA Client
    Copyright (C) 2021 Telescope Project
*/

// Basic stuff
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

// Networking + RDMA stuff
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <rdma/rsocket.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <infiniband/verbs.h>

// X11 stuff
#include <X11/Xatom.h>

#include "receive_shm.h"
#include "input_xcb.h"
#include "../common/rdma_common.h"

#define INLINE_BUFSIZE 32
#define TOTAL_WR 3
#define TOTAL_SGE 1

#define __T_DEBUG__

static bool use_tcp = true;
static int num_tests = 0;

/* Thread argument parameters */

typedef struct {
    struct rdma_cm_id   *connid;
    xcb_connection_t    *c;
    xcb_screen_t        *screen;
    Display             *display;
    Window              x11_window;
} mouse_param;

typedef struct {
    struct rdma_cm_id   *connid;
    struct ibv_mr       *ibv_mr;
    void                *buffer;
    Display             *display;
    Window              x11_window;
    Visual              *visual;
    GC                  gc;
    int                 tcp_server_fd;
} screen_param;

/* Mouse and keyboard capture and sending threads */

void send_mouse_data(struct rdma_cm_id * connid, char *buff, struct ibv_mr *send_mr) 
{  
    printf("Sending...");
    fflush(stdout);
    struct ibv_wc wc;
    int ret = rdma_post_send(connid, NULL, buff, INLINE_BUFSIZE, send_mr, IBV_SEND_INLINE);
    if (ret)
    {
        fprintf(stderr, "Error occurred: %s @ %d\n", strerror(errno), num_tests);
    }
    while ((ret = rdma_get_send_comp(connid, &wc)) == 0)
    {
        printf("Waiting for completed send\n");
    }
    if (ret < 0)
    {
        fprintf(stderr, "Error while waiting for completion: %s\n", strerror(errno));
    }
    printf(" flushed\n");
    fflush(stdout);
}

void *capture_mouse_keyboard(void *args)
{
    
    mouse_param *mouse_kb = (mouse_param*) args;
    int ret;
    
    // Resize the CQ to be able to handle fast mouse packets
    ret = ibv_resize_cq(mouse_kb->connid->send_cq, 10000);
    if (ret)
    {
        fprintf(stderr, "CQ resize failed: %s\n", strerror(errno));
    }
    
    // Allocate memory so that this segment can be transferred over RDMA
    char *send_buf = calloc(INLINE_BUFSIZE, 1);
    struct ibv_mr *send_mr = rdma_reg_msgs(mouse_kb->connid, send_buf, INLINE_BUFSIZE);
    
    int cenx, ceny;  
    int flag = 0;
    int left, right, upper, bottom;  //changed
    char *mouse_mov = malloc(9);
    char *mouse_but = malloc(6);
    char *keyboard = malloc(6);

    //xcb_xfixes_query_version(mouse_kb->c, 4, 0);
    xcb_xfixes_show_cursor(mouse_kb->c, mouse_kb->x11_window);
    xcb_generic_event_t *event;
    
    // Store the mouse movement deltas
    int16_t pre_x, pre_y, new_x, new_y, deltax, deltay = 0;

    while (event = xcb_wait_for_event (mouse_kb->c))
    {
        memset(send_buf, 0, INLINE_BUFSIZE);
        *(int *) send_buf = T_INLINE_DATA_HID;
        switch (event->response_type & ~0x80)
        {
            case XCB_MOTION_NOTIFY:
            {
                xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *) event;
                get_param(mouse_kb->c, mouse_kb->x11_window, &cenx, &ceny, &left, &right, &upper, &bottom);
                
                new_x = motion->event_x;	
                new_y = motion->event_y;

                deltax = new_x - pre_x;
                deltay = new_y - pre_y;

                pre_x = motion->event_x;
                pre_y = motion->event_y;

                if ((left >= pre_x)||(pre_x>=right)||(upper>=pre_y)||(bottom<=pre_y))
                {    
                    printf("warp\n");
                    int pre_y = ceny;
                    int new_x = cenx;
                    int new_y = ceny;
                    int deltax = 0;
                    int deltay = 0;
                    flag = 1;
                    xcb_warp_pointer(mouse_kb->c, mouse_kb->x11_window, mouse_kb->x11_window, 0,0,0,0, cenx, ceny);
                }
                else
                {
                    if(flag == 0){
                        printf("(%d : %d)\n", deltax, deltay);

                        flag = 1;
                        printf("(%d : %d)\n", deltax, deltay);
                        send_buf[4] = (char) 0;
                        *(int *) (&send_buf[5]) = deltax;
                        *(int *) (&send_buf[9]) = deltay;
                        send_mouse_data(mouse_kb->connid, send_buf, send_mr); 
                    }
                    else
                    {
                        flag = 0;
                    }
                }
                break;
            }
            case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t *bp = (xcb_button_press_event_t *)event;
                printf ("Button %d pressed\n", bp->detail );
                send_buf[4] = (char) 1;

                int *ptr5 = (int *)(&send_buf[5]);
                *ptr5 = bp->detail;

                send_buf[9] = (char) 1;
                send_mouse_data(mouse_kb->connid, send_buf, send_mr); 
                break;
            }			
            case XCB_BUTTON_RELEASE:
            {
                xcb_button_release_event_t *br = (xcb_button_release_event_t *)event;
                printf ("Button %d released\n", br->detail);
                send_buf[4] = (char) 1;

                int *ptr6 = (int *)(&send_buf[5]);
                *ptr6 = br->detail;

                send_buf[9] = (char) 0;
                send_mouse_data(mouse_kb->connid, send_buf, send_mr); 

                break;
            }
            case XCB_KEY_PRESS:
            {
                xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;
                printf ("Key pressed in window %d\n", kp->detail);
                send_buf[4] = (char) 2;

                int *ptr = (int *)(&send_buf[5]);
                *ptr =  kp->detail;

                send_buf[9] = (char) 1;
                send_mouse_data(mouse_kb->connid, send_buf, send_mr);

                break;
            }
            case XCB_KEY_RELEASE:
            {
                xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
                printf ("Key released in window %d\n", kr->detail);
                send_buf[4] = (char) 2;

                int *ptr = (int *)(&send_buf[5]);
                *ptr =  kr->detail;

                send_buf[9] = (char) 0;
                send_mouse_data(mouse_kb->connid, send_buf, send_mr);

                break;
            }
            case XCB_ENTER_NOTIFY:
            {
                xcb_xfixes_hide_cursor(mouse_kb->c, mouse_kb->screen->root);
                printf("enter\n");
                break;
            }
            case XCB_LEAVE_NOTIFY:
            {
                xcb_xfixes_show_cursor(mouse_kb->c, mouse_kb->screen->root);
                printf("leave\n");
                break;
            }
            default:
            {
                printf("Unknown event: %d\n", event->response_type);
                break;
            }
        }
    }
}

/* Screen receive and draw thread */

void *screen_recv(void *void_args) 
{
    
    screen_param *args = (screen_param *) void_args;
    struct ibv_wc wc;
    struct ibv_mr *recv_mr, *send_mr;
    int ret;

    // This packet is sent to request a buffer write from the server
    char *send_buff = calloc(1, INLINE_BUFSIZE);
    *(int *) (send_buff) = T_REQ_BUFFER_WRITE;

    char *recv_buff = calloc(1, INLINE_BUFSIZE);

    // XImage *ximage = XCreateImage(args->display, args->visual, 32, ZPixmap,
    //     0, args->buffer, WIDTH, HEIGHT, 32, 0);

    send_mr = rdma_reg_msgs(args->connid, (void *) send_buff, INLINE_BUFSIZE);
    if (!send_mr)
    {
        fprintf(stderr, "Could not register inline memory region.\n");
    }
    recv_mr = rdma_reg_msgs(args->connid, (void *) recv_buff, INLINE_BUFSIZE);
    if (!recv_mr)
    {
        fprintf(stderr, "Could not register inline memory region.\n");
    }
    for (int i = 0; i < INLINE_BUFSIZE; i++)
    {
        fprintf(stderr, "%hhx", *((char *) send_buff + i));
    }
    fprintf(stderr, "\n");
    int times = 0;

    struct timeval t0, t1;

    while (1)
    {

        // Wait for the server to send a confirmation back that the data has
        // been sent successfully.

        if (use_tcp)
        {
            ret = read(args->tcp_server_fd, recv_buff, INLINE_BUFSIZE);
            if (ret < 0)
            {
                continue;
            }
        }
        else
        {
            ret = rdma_post_recv(args->connid, NULL, recv_buff, INLINE_BUFSIZE, recv_mr);
            if (ret)
            {
                fprintf(stderr, "Error occurred while posting receive request: %s\n", strerror(errno));
            }

            #ifdef __T_DEBUG__
            printf("Posted receive request ...\n");
            #endif

            while ((ret = rdma_get_recv_comp(args->connid, &wc)) == 0)
            {
                printf("Waiting for placement ... \n");
            }

            if (wc.status)
            {
                fprintf(stderr, "Fatal error @ #2: %d\n", wc.status);
                return NULL;
            }
        }
        
        // Now that the buffer is in memory we can copy it into X11 for later use

        #ifdef __T_DEBUG__
        printf("Received buffer ...\n");
        #endif 
        for (int i = 0; i < 50; i++)
        {
            fprintf(stderr, "%hhx", *((char *) args->buffer + i));
        }
        fprintf(stderr, "\n");
        noshm_putimage(args->buffer, args->display, args->visual, args->x11_window, args->gc);
        
        #ifdef __T_DEBUG__
        printf("Put the image ... %d\n", ++times);
        #endif
    }
} 

/* Control thread */

int main(int argc, char *argv[]) 
{    
    int ret;
    // Use rdma_cm to create an RDMA RC connection.
    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;
    
    hints.ai_port_space = RDMA_PS_TCP;  // Allocate a TCP port for the connection setup
    
    printf("Attempting to connect to %s:%s\n", argv[1], argv[2]);
    ret = rdma_getaddrinfo(argv[1], argv[2], &hints, &host_res);
    if (ret)
    {
        fprintf(stderr, "Could not resolve host.\n");
        return 1;
    }

    struct rdma_cm_id *connid = malloc(sizeof(struct rdma_cm_id));
    new(struct ibv_qp_init_attr, init_attr);
    
    // Allow the same QP to be used by two threads with 256 bytes
    // allocated for inline data, such as HID input. Also create
    // it as reliable connected (TCP equivalent)
    init_attr.cap.max_send_wr = TOTAL_WR;
    init_attr.cap.max_recv_wr = TOTAL_WR;
    init_attr.cap.max_send_sge = TOTAL_SGE;
    init_attr.cap.max_recv_sge = TOTAL_SGE;
    init_attr.cap.max_inline_data = INLINE_BUFSIZE;
    init_attr.qp_type = IBV_QPT_RC;
    init_attr.qp_context = connid;
    init_attr.sq_sig_all = 1;

    ret = rdma_create_ep(&connid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection.\n");
        return 1;
    }
    if (!(init_attr.cap.max_inline_data >= INLINE_BUFSIZE
        && init_attr.qp_type == IBV_QPT_RC))
    {
        fprintf(stderr, 
                "Connection does not support features required by Telescope:\n"
                "Max inline data: %d\t(req. %d)\n"
                "QP Type:         %d\t(req. %d - IBV_QPT_RC)\n",
                init_attr.cap.max_inline_data, INLINE_BUFSIZE, 
                init_attr.qp_type, IBV_QPT_RC
        );
        rdma_destroy_ep(connid);
        return 1;
    }

    int send_flags;

    ret = rdma_connect(connid, NULL);
    if (ret)
    {
        fprintf(stderr, "Could not connect to server.\n");
        return 1;
    }

    Status mt_status;
    mt_status = XInitThreads();
    if (!mt_status) {
        fprintf(stderr, "Could not initialize X11 for concurrency.\n");
    } else {
        printf("Initialized X11 multithreading (%d)\n", mt_status);
    }

    int win_b_color;
    int win_w_color;
    XEvent xev;
    Window window;
    GC gc;
    Visual *visual;
    XImage *ximage;
    
    /* Set up X11 and XCB display connections */

    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Could not open X display\n");
        exit(1);
    }
    
    xcb_connection_t *c = XGetXCBConnection(display);
    if (!c) {
        fprintf(stderr, "Could not establish XCB connection\n");
        exit(1);
    }

    win_b_color = BlackPixel(display, DefaultScreen(display));
    win_w_color = BlackPixel(display, DefaultScreen(display));
    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 
        0, 0, WIDTH, HEIGHT, 0, win_b_color, win_w_color);
    visual = DefaultVisual(display, 0);
    Atom window_type = XInternAtom(display, "_NET_WM_STATE", false);
    long value = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", false);
    XChangeProperty(display, window, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);       
    XSelectInput(display, window, XCB_EVENT_MASK_POINTER_MOTION
                                | XCB_EVENT_MASK_BUTTON_PRESS 
                                | XCB_EVENT_MASK_BUTTON_RELEASE
                                | XCB_EVENT_MASK_KEY_PRESS
                                | XCB_EVENT_MASK_KEY_RELEASE
                                | XCB_EVENT_MASK_ENTER_WINDOW
                                | XCB_EVENT_MASK_LEAVE_WINDOW);
    XStoreName(display, window, "X11 window");
    XMapWindow(display, window);
    XFlush(display);
    
    gc = XCreateGC(display, window, 0, NULL);
    
    /* Create connection objects for RDMA + TCP */
        //added area2
    //added area
    pthread_t thread1, thread2;
    int th1, th2;
    //conn_param *conn_config = calloc(1, sizeof(conn_param));
    mouse_param *mouse_config = calloc(1, sizeof(mouse_param));
    screen_param* screen_config = calloc(1, sizeof(screen_param));

    mouse_config->c = c;
    mouse_config->x11_window = window;
    mouse_config->display = display;
    mouse_config->connid = connid;

    screen_config->connid = connid;
    screen_config->display = display;
    screen_config->visual = visual;
    screen_config->x11_window = window;
    screen_config->gc = gc;

    // Register the memory region to allow remote writes
    // for the image data.
    
    int width = 1920;
    int height = 1080;
    int bpp = 32;
    uint64_t image_bytes = width * height * (bpp / 8);
    char *hello_buf = calloc(INLINE_BUFSIZE, 1);
    
    bool use_shm = false;

    struct ibv_mr *image_mr;
    struct ibv_wc wc;
    
    if (use_shm)
    {
        // Create X11 image and register MR for data input
    
        XImage *eggs_image;
        struct shmimage src;

        initimage(&src);

        if(!(eggs_image = shm_create_ximage(display, &src, WIDTH, HEIGHT)))
        {
            XCloseDisplay(display);
            return 1;
        }
        
        // First we need to tell the server what memory region we want the client
        // to use, so we send the required data, which is the image size, the buffer's
        // size, the key to access the data, and the address in this client's memory.

        image_mr = ibv_reg_mr(connid->pd, (void *) src.shminfo.shmaddr, image_bytes, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
        if (!image_mr)
        {
            fprintf(stderr, "Error registering memory region: %s", strerror(errno));
            return 1;
        }
        screen_config->buffer = src.shminfo.shmaddr;
    }
    else
    {

        // If SHM is disabled, create a memory region without registration
        // as a shared memory segment. The system will fall back to the
        // slower routines, but still allow RDMA writes to the image buffer.

        unsigned int *buffer = calloc(image_bytes, 1);

        image_mr = ibv_reg_mr(connid->pd, (void *) buffer, image_bytes, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
        if (!image_mr)
        {
            fprintf(stderr, "Error registering memory region: %s", strerror(errno));
            return 1;
        }
        screen_config->buffer = buffer;
        
    }

    // Allocate the memory we need to send the memory region message
    // Generally inline buffers do not need to be registered by
    // rdma_reg_msgs, but it is registered in case inline sends are disabled.

    struct ibv_mr *send_mr;
    char *send_buf = calloc(1, INLINE_BUFSIZE);

    if (!send_buf)
    {
        fprintf(stderr, "Could not allocate send buffer.\n");
        return 1;
    }

    send_mr = rdma_reg_msgs(connid, send_buf, INLINE_BUFSIZE);
    if (!send_mr)
    {
        fprintf(stderr, "Could not allocate sending MR.\n");
        return 1;
    }
    
    // Prepare memory information to be sent to the server for
    // RDMA writes
    T_InlineBuff *mem_msg = (T_InlineBuff *) send_buf;
    mem_msg->data_type = T_INLINE_DATA_CLI_MR;
    mem_msg->addr = image_mr->addr;
    mem_msg->size = image_bytes;
    mem_msg->rkey = image_mr->rkey;
    mem_msg->numbufs = 1;

    uint32_t dtype, rkey, nbufs;
    uint64_t addr, size;

    printf("Send buffer and mem_msg\n");
    printb(send_buf, INLINE_BUFSIZE);
    printb((char *) mem_msg, sizeof(T_InlineBuff));
    printf("-----------------------\n");

    printf( "------- Client information -------\n"
            "(Type: %20lu) (Addr: %20lu)\n"
            "(Size: %20lu) (Rkey: %20lu)\n"
            "(Bufs: %20lu)\n" 
            "----------------------------------\n",
            (uint64_t) mem_msg->data_type, (uint64_t) mem_msg->addr, (uint64_t) mem_msg->size,
            (uint64_t) mem_msg->rkey, (uint64_t) mem_msg->numbufs);

    // Send the memory region information so the server knows where to perform
    // the RDMA write operations to
    
    ret = rdma_post_send(connid, NULL, (void *) send_buf, INLINE_BUFSIZE, image_mr, IBV_SEND_INLINE);
    while ((ret = rdma_get_send_comp(connid, &wc)) == 0)
    {
        printf("Sending post send data...\n");
    }

    bool use_tcp = true;
    int connfd = -1;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    if (use_tcp)
    {

        connfd = socket(AF_INET, SOCK_STREAM, 0);
        if (connfd < 0)
        {
            fprintf(stderr, "Failed to create the TCP socket!\n");
            return 1;
        }
        printf("Created TCP event notification socket.\n");

        int flag = 1;
        ret = setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
        if (ret < 0)
        {
            fprintf(stderr, "Cannot set socket options.\n");
            return 1;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(argv[1]);
        server_addr.sin_port = htons(9999);

        ret = connect(connfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
        if (ret != 0)
        {
            fprintf(stderr, "Could not connect to the server.\n");
            return 1;
        }

    }

    screen_config->tcp_server_fd = connfd;
    //conn_config->connid = connid;

    //th1 = pthread_create(&thread1, NULL, capture_mouse_keyboard, (void *) mouse_config);
    //th2 = pthread_create(&thread2, NULL, poll_completion, (void *) conn_config);
    th2 = pthread_create(&thread2, NULL, screen_recv, (void*) screen_config);

    //pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("All threads have exited.\n");

    exit(0);

} 
