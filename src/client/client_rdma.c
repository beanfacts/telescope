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

#include "../common/rdma_common.h"

#define MAX 10000000 
#define PORT 6969 
#define SA struct sockaddr 
#define WIDTH 1920
#define HEIGHT 1080

pthread_mutex_t lock;
#define INLINE_BUFSIZE 24
#define TOTAL_WR 1
#define TOTAL_SGE 1

static struct rdma_cm_id *connid;

typedef struct {
    xcb_connection_t *c;
    xcb_screen_t *screen;
    Window x11_window;
    Display *display;
    struct rdma_cm_id * connid;
} mouse_param;

typedef struct {
    int sockfd;
    Display* display;
    Visual *visual;
    Window x11_window;
    GC gc;
} screen_param;

// static void setCursor (xcb_connection_t*, xcb_screen_t*, xcb_window_t, int);
static void testCookie(xcb_void_cookie_t, xcb_connection_t*, char *); 

static void testCookie (xcb_void_cookie_t cookie, xcb_connection_t *connection, char *errMessage)
{   
    xcb_generic_error_t *error = xcb_request_check (connection, cookie);
    if (error) {
        fprintf (stderr, "ERROR: %s : %d\n", errMessage , error->error_code);
        xcb_disconnect (connection);
        exit (-1);
    }
}   
/*
static void setCursor (xcb_connection_t *connection,
    xcb_screen_t     *screen,
    xcb_window_t      window,
    int               cursorId)
{
    xcb_font_t font = xcb_generate_id (connection);
    xcb_void_cookie_t fontCookie = xcb_open_font_checked (connection,
                                                            font,
                                                            strlen ("cursor"),
                                                            "cursor" );
    testCookie (fontCookie, connection, "can't open font");
    xcb_cursor_t cursor = xcb_generate_id (connection);
    xcb_create_glyph_cursor (connection,
                                cursor,
                                font,
                                font,
                                cursorId,
                                cursorId + 1,
                                0, 0, 0, 0, 0, 0 );

    xcb_gcontext_t gc = xcb_generate_id (connection);

    uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
    uint32_t values_list[3];
    values_list[0] = screen->black_pixel;
    values_list[1] = screen->white_pixel;
    values_list[2] = font;

    xcb_void_cookie_t gcCookie = xcb_create_gc_checked (connection, gc, window, mask, values_list);
    testCookie (gcCookie, connection, "can't create gc");

    mask = XCB_CW_CURSOR;
    uint32_t value_list = cursor;
    xcb_change_window_attributes (connection, window, mask, &value_list);

    xcb_free_cursor (connection, cursor);

    fontCookie = xcb_close_font_checked (connection, font);
    testCookie (fontCookie, connection, "can't close font");
}
*/
void get_center(xcb_connection_t *c, xcb_window_t window, int *ret_cenx, int *ret_ceny) {

    xcb_get_geometry_cookie_t cookie;
    xcb_get_geometry_reply_t *reply;

    cookie = xcb_get_geometry_unchecked(c, window);
    if ((reply = xcb_get_geometry_reply(c, cookie, NULL))) {
        *ret_cenx = (reply->width)/2;
        *ret_ceny = (reply->height)/2;   
    }
    free(reply);
}

// todo: message invalid, fix later
void send_input(struct rdma_cm_id * connid, char *buff, struct ibv_mr *send_mr) 
{  
    // if (size == 9)
    // {
    //     printf("buf ->");
    //     for(int i=0; i < size; i++)
    //     {
    //         printf("(%d) ", (unsigned char) *(buff+i));
    //     }
    //     printf("\n");
    // }
    // for(int i=0; i < size; i++){
    //     printf("%hhx", *(buff+i));
    // }
    // printf("\n");
    //write(sockfd, buff, size);
    struct ibv_wc wc;
    int ret = rdma_post_send(connid, NULL, buff, INLINE_BUFSIZE, send_mr, IBV_SEND_INLINE);
    if (ret)
    {
        fprintf(stderr, "Error occurred: %s\n", strerror(errno));
    }
    while ((ret = rdma_get_send_comp(connid, &wc)) == 0)
    {
        printf("Waiting for completed send\n");
    }
    if (ret < 0)
    {
        fprintf(stderr, "Error while waiting for completion: %s\n", strerror(errno));
    }
}

void *capture_mouse_keyboard(void *args) {
    
    
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
    *(int *) send_buf = T_INLINE_DATA_HID;
    struct ibv_mr *send_mr = rdma_reg_msgs(connid, send_buf, INLINE_BUFSIZE);
    
            
    int cenx, ceny;  
    int flag = 0;
    
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
        switch (event->response_type & ~0x80)
        {
            case XCB_MOTION_NOTIFY:
            {
                xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *) event;
                get_center(mouse_kb->c, mouse_kb->x11_window, &cenx, &ceny);
                
                // Calculate the amount that the mouse move
                new_x = motion->event_x;	
                new_y = motion->event_y;

                deltax = new_x - pre_x;
                deltay = new_y - pre_y;

                pre_x = motion->event_x;
                pre_y = motion->event_y;
                
                if (((deltax != 0) && (deltay != 0)) && (flag == 0)) {
                    
                    flag = 1;
                    printf("(%d : %d)\n", deltax, deltay);
                    
                    send_buf[4] = (char) 0;
                    *(int *) (&send_buf[5]) = deltax;
                    *(int *) (&send_buf[9]) = deltay;

                    // rdma_post_send(connid, NULL, hello, INLINE_BUFSIZE, send_mr, IBV_SEND_INLINE);

                    send_input(mouse_kb->connid, send_buf, send_mr); // THIS ONE IS CORRECT I GUESS!!!
                    //memset(send_buf, 0, 9);
                }
                else if (flag == 1)
                {
                    flag = 0;
                }
                xcb_warp_pointer(mouse_kb->c, mouse_kb->x11_window, mouse_kb->x11_window, 0, 0, 0, 0, cenx, ceny); 
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
                send_input(mouse_kb->connid, send_buf, send_mr); 
               // memset(send_buf, 0,6);  
            }			
            case XCB_BUTTON_RELEASE:
            {
                xcb_button_release_event_t *br = (xcb_button_release_event_t *)event;
                printf ("Button %d released\n", br->detail);
                send_buf[4] = (char) 1;

                int *ptr6 = (int *)(&send_buf[5]);
                *ptr6 = br->detail;

                send_buf[9] = (char) 0;
                send_input(mouse_kb->connid, send_buf, send_mr); 
                //memset(send_buf, 0, 6);
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
                send_input(mouse_kb->connid, send_buf, send_mr);
                //memset(keyboard, 0, 6);
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
                send_input(mouse_kb->connid, keyboard, send_mr);
               // memset(keyboard, 0,6);
                break;
            }
            /*
            case XCB_ENTER_NOTIFY:{
                xcb_xfixes_hide_cursor(mouse_kb->c, mouse_kb->screen->root);
                printf("enter\n");
                break;
            }
            case XCB_LEAVE_NOTIFY:{
                xcb_xfixes_show_cursor(mouse_kb->c, mouse_kb->screen->root);
                // setCursor (mouse_kb->c, mouse_kb->screen, mouse_kb->xcb_window, 58);
                printf("leave\n");
                break;
            }
            */
            default:
            {
                printf("Unknown event: %d\n", event->response_type);
                break;
            }
        }
    }  
}

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
    struct ibv_wc wc;

    ret = rdma_connect(connid, NULL);

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
    mouse_param *mouse_config = calloc(1, sizeof(mouse_param));
    screen_param* screen_config = calloc(1, sizeof(screen_param));

    mouse_config->c = c;
    mouse_config->x11_window = window;
    mouse_config->display = display;
    mouse_config->connid = connid;
    // screen_config->sockfd = sockfd;
    // screen_config->display = display;
    // screen_config->visual = visual;
    // screen_config->x11_window = window;
    // screen_config->gc = gc;

    th1 = pthread_create(&thread1, NULL, capture_mouse_keyboard, (void*) mouse_config);
    //th2 = pthread_create(&thread2, NULL, func, (void*) screen_config);

    pthread_join(thread1, NULL);
   // pthread_join(thread2, NULL);

    printf("All threads have exited.\n");

    exit(0);

} 
