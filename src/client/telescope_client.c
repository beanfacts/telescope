// Basic stuff
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// Networking + RDMA stuff
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <rdma/rsocket.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <infiniband/verbs.h>

// XCB stuff
#include <xcb/xproto.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>

// X11 stuff
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xatom.h>

// Shared memory extensions
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#include "rdma_common.h"

#define MAX 10000000 
#define PORT 6969  
#define SA struct sockaddr 
#define WIDTH 1920
#define HEIGHT 1080

typedef struct {
    xcb_connection_t *c;
    xcb_screen_t *screen;
    Window x11_window;
    Display *display;
    int sockfd;
} mouse_param;

typedef struct {
    struct ibv_qp_init_attr init_attr;  // RDMA QP information
    Display* display;                   // X11 display
    Visual *visual;                     // X11 root visual
    Window x11_window;                  // X11 window
    GC gc;                              // X11 graphics context
} screen_param;

pthread_mutex_t lock;

static void testCookie (xcb_void_cookie_t cookie, xcb_connection_t *connection,
    char *errMessage)
{   
    xcb_generic_error_t *error = xcb_request_check(connection, cookie);
    if (error) {
        fprintf (stderr, "ERROR: %s : %d\n", errMessage , error->error_code);
        xcb_disconnect (connection);
        exit (-1);
    }
}   


void get_center(xcb_connection_t *c, xcb_window_t window, int *ret_cenx, int *ret_ceny) {

    xcb_get_geometry_cookie_t cookie;
    xcb_get_geometry_reply_t *reply;

    cookie = xcb_get_geometry_unchecked(c, window);

    if ((reply = xcb_get_geometry_reply(c, cookie, NULL)))
    {
        *ret_cenx = (reply->width)/2;
        *ret_ceny = (reply->height)/2;   
    }

    free(reply);
}


void send_input(int sockfd, char *buff, int size) 
{ 
   
    if (size == 9){
        printf("buf ->");
        for(int i=0; i < size; i++){
            printf("(%d) ", (unsigned char) *(buff+i));
        }
        printf("\n");
    }

    for(int i=0; i < size; i++){
        printf("%hhx", *(buff+i));
    }

    printf("\n");
    write(sockfd, buff, size); 
} 


void *capture_mouse_keyboard(void *args) {

    mouse_param *mouse_kb = (mouse_param*) args;      
    int cenx, ceny; 
    xcb_xfixes_query_version(mouse_kb->c,4,0);
    xcb_xfixes_show_cursor(mouse_kb->c, mouse_kb->x11_window);
    int flag = 0;
    xcb_generic_event_t *event;
    int16_t pre_x, pre_y, new_x, new_y, deltax, deltay = 0;
    char *mouse_mov = malloc(9);
    char *mouse_but = malloc(6);
    char *keyboard = malloc(6);

    while(event = xcb_wait_for_event (mouse_kb->c))
    {
        switch (event->response_type & ~0x80)
        {
            case XCB_MOTION_NOTIFY:
            {
                xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
                get_center(mouse_kb->c, mouse_kb->x11_window, &cenx, &ceny);
                
                // Calculate the amount that the mouse move
                new_x = motion->event_x;	
                new_y = motion->event_y;
                deltax = new_x - pre_x;
                deltay = new_y - pre_y;
                pre_x = motion->event_x;
                pre_y = motion->event_y;
                
                if (((deltax != 0) && (deltay != 0)) && (flag == 0))
                {    
                    flag = 1;
                    printf("(%d : %d)\n", deltax, deltay);
                    mouse_mov[0] = (char) 0;
                    *(int *) (&mouse_mov[1]) = deltax;
                    *(int *) (&mouse_mov[5]) = deltay;
                    send_input(mouse_kb->sockfd, mouse_mov, 9); 
                    memset(mouse_mov, 0, 9);
                }
                else if (flag == 1)
                {
                    flag = 0;
                }
                xcb_warp_pointer(mouse_kb->c, mouse_kb->x11_window, mouse_kb->x11_window, 0, 0, 0, 0, cenx, ceny); 
                break;
            }

            case XCB_BUTTON_RELEASE:
            {
                xcb_button_release_event_t *br = (xcb_button_release_event_t *)event;
                printf ("Button %d released\n", br->detail);
                mouse_but[0] = (char) 1;
                int *ptr6 = (int *)(&mouse_but[1]);
                *ptr6 = br->detail;
                mouse_but[5] = (char) 0;
                send_input(mouse_kb->sockfd, mouse_but, 6); 
                memset(mouse_but, 0, 6);
                break;
            }

            case XCB_KEY_PRESS:
            {
                xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;           
                printf ("Key pressed in window %d\n", kp->detail);
                keyboard[0] = (char) 2;
                int *ptr = (int *)(&keyboard[1]);
                *ptr =  kp->detail;
                keyboard[5] = (char) 1;
                send_input(mouse_kb->sockfd, keyboard, 6);
                memset(keyboard, 0, 6);
                break;
            }

            case XCB_KEY_RELEASE: 
            {
                xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
                printf ("Key released in window %d\n", kr->detail);
                keyboard[0] = (char) 2;
                int *ptr = (int *)(&keyboard[1]);
                *ptr =  kr->detail;
                keyboard[5] = (char) 0;
                send_input(mouse_kb->sockfd, keyboard, 6);
                memset(keyboard, 0,6);
                break;
            }

            case XCB_ENTER_NOTIFY:
            {
                xcb_xfixes_hide_cursor(mouse_kb->c, mouse_kb->x11_window);
                printf("enter\n");
                break;              
            }

            case XCB_LEAVE_NOTIFY:
            {
                xcb_xfixes_show_cursor(mouse_kb->c, mouse_kb->x11_window);
                printf("leave\n");
                break;
                
            }

            case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t *bp = (xcb_button_press_event_t *)event;
                printf ("Button %d pressed\n", bp->detail );
                mouse_but[0] = (char) 1;
                int *ptr5 = (int *)(&mouse_but[1]);
                *ptr5 = bp->detail;
                mouse_but[5] = (char) 1;
                send_input(mouse_kb->sockfd, mouse_but, 6); 
                memset(mouse_but, 0,6);
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


XImage *create_ximage(Display *display, Visual *visual, int width, int height, char *image)
{
    return XCreateImage(display, visual, 24,ZPixmap, 0, image, width, height, 32, 0);
}


int putimage(char *image, Display *display, Visual *visual, Window window, GC gc)
{    
    XImage *ximage = create_ximage(display, visual, WIDTH, HEIGHT, image);
    XEvent event;
    bool exit = false;
    int r;
    r = XPutImage(display, window, gc, ximage, 0, 0, 0, 0, WIDTH, HEIGHT);
    printf("RES: %i\n", r);
    return 0;
}


void *user_setup(int connfd)
{
    // Request the available displays.
    T_MsgHeader *hdr = (T_MsgHeader *) calloc(1, sizeof(T_MsgHeader));
    hdr->msg_type = T_REQ_SCREEN_DATA
}


void *run_capture(void *args_void) 
{
    
    screen_param args = (screen_param *) args_void;
    // Set up client resources for RDMA operations
    int ret;
    
    // Get the available displays from the server
    T_List *dpy_list;
    T_ScreenData *dpy;
    dpy_list = get_displays(args.sockfd);
    if (dpy_list)
    {
        dpy = (T_ScreenData *) dpy_list->data;
        for (int i = 0; i < dpy_list->elements; i++)
        {
            printf(
                "X Screen %d @ %d,%d\n"
                "    Resolution: %d x %d @ %d bpp\n",
                dpy->screen_index, dpy->screen_offset_x, dpy->screen_offset_y,
                dpy->width, dpy->height, dpy->bits_per_pixel
            );
            dpy++;
        }
    }

    // Allow the user to select the display to stream from the server
    int screen_index = -1;
    int ret = 0;
    while (!ret && screen_index < 0)
    {
        printf("Select screen index: ");
        ret = scanf("%d", &screen_index);
    }
    
    /*  Before sending the request to the server, the memory region we want the
        server to write to has to be pinned and mapped for rsockets to use.
        The client creates a buffer twice the required size, to allow the server
        to write data while the client is busy rendering the image, without the
        risk of image data corruption. Each image also has a header with some
        metadata about the image, as a confirmation for the client.
    */
    dpy = (T_ScreenData *) dpy_list->data;
    int buf_size = -1;
    for (int i = 0; i < dpy_list->elements; i++)
    {
        if (dpy->screen_index == screen_index)
        {
            buf_size = dpy->height * dpy->width * (dpy->bits_per_pixel / 8) * 2;
            buf_size += sizeof(T_ScreenData);
            break;
        }
        dpy++;
    }

    // todo: perform memcpy!!!! then free!!

    /*  The buffer size is pinned and mapped for RDMA remote operations.
        Note the client must have permissions to pin the amount of memory
        required for these operations.
    */
    if (buf_size)
    {
        void *buf = malloc(buf_size);
        if (buf)
        {
            // Let rsocket choose an available offset to be used
            off_t frame_offset = riomap(args.sockfd, buf, buf_size, PROT_WRITE, 0, -1);
            if (frame_offset)
            {
                /*  To service a client, the server must first allocate its own buffer
                    for the storage of image data. The display with specified index
                    is requested for the server to prepare.
                */
                ret = prepare_image(args.sockfd, dpy->screen_index, CHR_ASIS);
                if (!ret)
                {
                    fprintf(stderr, "Server did not respond to image preparation request.\n");
                }
            }
        }
        else
        {
            fprintf(stderr, "Could not allocate %d kiB of memory for image data.\n", buf_size / 1024);
        }
    }

    /*  Now that the server is prepared, the client has to prepare to receive
        the data requested. First, X11 on the client side is prepared to receive
        the incoming image data. Note that in this case, the format XYBitmap must
        be used to eliminate the need to transfer the alpha plane.
    */
    XShmSegmentInfo xshm_info;
    XImage *img = XShmCreateImage(
        args.display, args.visual, 24, XYBitmap,
        NULL, &xshm_info, dpy->width, dpy->height);
    xshm_info.shmid = shmget(IPC_PRIVATE, dpy->width * dpy->height * (dpy->bits_per_pixel / 8), 0700);
    xshm_info.shmaddr = shmat(xshm_info.shmid, 0, 0);
    xshm_info.readOnly = False;

    // todo: put the image then save it

    screen_param *cap_screen = (screen_param *) args;
    char *buff=calloc(MAX,1); 
    int n; 
    int length;
    int framebytes = 0;
    int maxbytes = WIDTH*HEIGHT*4;
    while (1) {
        length = rread(cap_screen->sockfd, buff+framebytes , maxbytes-framebytes);
        if (length > 0) {
            printf("Reading %d\n", framebytes);
            framebytes = framebytes + length;
            if (framebytes >= maxbytes) {
                printf("> %d", framebytes);
                framebytes = 0;
                putimage(buff, cap_screen->display, cap_screen->visual, cap_screen->x11_window, cap_screen->gc);
                memset(buff,0, MAX); 
            }
        }
        else
        {
            //fprintf(stderr, "Error reading from socket.\n");
        }
    } 
} 


int main(int argc, char *argv[]) 
{   
    
    if (argc != 3){
        printf( "Invalid argument count"
                "Usage: %s <ip> <port>", argv[0]);
        return 1;
    }    
        
    int ret;

    // Prepare X11 for multithreading
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
    
    // Set up X11 and XCB display connections
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Could not open X display\n");
        return 1;
    }
    
    xcb_connection_t *c = XGetXCBConnection(display);
    if (!c) {
        fprintf(stderr, "Could not establish XCB connection\n");
        return 1;
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
    
    // Use rdma_cm to create an RDMA RC connection.
    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;

    hints.ai_flags = RAI_PASSIVE;       // According to rdma_cm docs, allows both sides to transmit data
    hints.ai_port_space = RDMA_PS_TCP;  // Allocate a TCP port for the connection setup
    ret = rdma_getaddrinfo(argv[1], argv[2], &hints, &host_res);
    if (ret)
    {
        fprintf(stderr, "Could not resolve host.\n");
        return 1;
    }

    new(struct ibv_qp_init_attr, init_attr);
    struct rdma_cm_id *connid;
    
    // Allow the same QP to be used by two threads with 256 bytes
    // allocated for inline data, such as HID input. Also create
    // it as reliable connected (TCP equivalent)
    init_attr.cap.max_send_wr = 2;
    init_attr.cap.max_recv_wr = 2;
    init_attr.cap.max_send_sge = 2;
    init_attr.cap.max_recv_sge = 2;
    init_attr.cap.max_inline_data = 256;
    init_attr.qp_type = IBV_QPT_RC;

    ret = rdma_create_ep(&connid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection.\n");
    }
    if (!(init_attr.cap.max_inline_data >= 256 && init_attr.cap.max_send_sge >= 2
        && init_attr.qp_type == IBV_QPT_RC))
    {
        fprintf(stderr, 
            "Connection does not support features required by Telescope:\n"
            "Max inline data: %d\t(req. %d)\n"
            "Max send SGEs:   %d\t(req. %d)\n"
            "QP Type:         %d\t(req. %d)\n"
        );
        rdma_destroy_ep(connid);
        return 1;
    }

    // The RDMA connection is ready for use.
    // Since RDMA CM is thread safe, there is no need for extra
    // synchronization between the two different threads.
    mouse_param *mouse_config = calloc(1, sizeof(mouse_param));
    screen_param* screen_config = calloc(1, sizeof(screen_param));

    screen_config->sockfd = rdma_fd;
    screen_config->display = display;
    screen_config->visual = visual;
    screen_config->x11_window = window;
    screen_config->gc = gc;

    mouse_config->c = c;
    mouse_config->x11_window = window;
    mouse_config->sockfd = tcp_fd;
    mouse_config->display = display;

    /*
    // Create a TCP connection.
    
    int tcp_fd;
    struct sockaddr_in tcp_addr, cli;

    // Configure the socket to not buffer outgoing data.
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(rdma_fd, SOL_TCP, TCP_NODELAY, (void *) &optval, sizeof(optval));
    if (tcp_fd == -1)
    { 
        printf("Could not create TCP socket.\n"); 
        exit(1); 
    } 
    printf("Created TCP Socket - FD: %d\n", rdma_fd);

    // Attempt a connection to the server.
    printf("Trying to connect to %s:%d (TCP)\n", argv[1], atoi(argv[2]));
    memset(&tcp_addr,0, sizeof(tcp_addr)); 
    
    tcp_addr.sin_family = AF_INET; 
    tcp_addr.sin_addr.s_addr = inet_addr(argv[1]); 
    tcp_addr.sin_port = htons(atoi(argv[2]) + 1); 
    
    if (rconnect(tcp_fd, (SA*)&tcp_addr, sizeof(tcp_addr)) != 0)
    { 
        printf("TCP connection with the server failed.\n"); 
        exit(1); 
    }

    //
    pthread_t thread1, thread2;
    int th1, th2;

    mouse_param *mouse_config = calloc(1, sizeof(mouse_param));
    screen_param* screen_config = calloc(1, sizeof(screen_param));

    screen_config->sockfd = rdma_fd;
    screen_config->display = display;
    screen_config->visual = visual;
    screen_config->x11_window = window;
    screen_config->gc = gc;

    mouse_config->c = c;
    mouse_config->x11_window = window;
    mouse_config->sockfd = tcp_fd;
    mouse_config->display = display;

    user_setup(rdma_fd);
    
    th1 = pthread_create(&thread1, NULL, capture_mouse_keyboard, (void*) mouse_config);
    th2 = pthread_create(&thread2, NULL, run_capture, (void*) screen_config);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("All threads have exited.\n");

    exit(0);
    */
} 
