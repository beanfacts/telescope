// Basic stuff
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// Networking + RDMA stuff
// #include <mime/multipart.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <rdma/rsocket.h>

// XCB stuff
#include <xcb/xproto.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>

// X11 stuffs
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xatom.h>

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
    int connfd;
} mouse_param;

typedef struct {
    int sockfd;
    Display* display;
    Visual *visual;
    Window x11_window;
    GC gc;
} screen_param;


pthread_mutex_t lock;

static void setCursor (xcb_connection_t*, xcb_screen_t*, xcb_window_t, int);
static void testCookie(xcb_void_cookie_t, xcb_connection_t*, char *); 

static void testCookie (xcb_void_cookie_t cookie, xcb_connection_t *connection,
    char *errMessage)
{   
    xcb_generic_error_t *error = xcb_request_check (connection, cookie);
    if (error) {
        fprintf (stderr, "ERROR: %s : %d\n", errMessage , error->error_code);
        xcb_disconnect (connection);
        exit (-1);
    }
}   


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
    
    while(event = xcb_wait_for_event (mouse_kb->c)) {

        switch (event->response_type & ~0x80)
        {
            case XCB_MOTION_NOTIFY: {
            
            xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
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

                mouse_mov[0] = (char) 0;
                *(int *) (&mouse_mov[1]) = deltax;
                *(int *) (&mouse_mov[5]) = deltay;

                send_input(mouse_kb->sockfd, mouse_mov, 9); 
                memset(mouse_mov, 0, 9);
            }
            else if(flag == 1){
                flag = 0;
            }

            xcb_warp_pointer(mouse_kb->c, mouse_kb->x11_window, mouse_kb->x11_window, 0,0,0,0, cenx, ceny); 
            break;
                
            }

            case XCB_BUTTON_RELEASE: {
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

            case XCB_KEY_PRESS: {
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

            case XCB_KEY_RELEASE: {
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

            // todo: will fix later
            
            case XCB_ENTER_NOTIFY:{
                /*xcb_xfixes_hide_cursor(mouse_kb->c, mouse_kb->screen->root);
                printf("enter\n");
                break;*/
                break;
            }

            case XCB_LEAVE_NOTIFY:{
                /*xcb_xfixes_show_cursor(mouse_kb->c, mouse_kb->screen->root);
                setCursor (mouse_kb->c, mouse_kb->screen, mouse_kb->xcb_window, 58);
                
                printf("leave\n");
                break;*/
                break;
            }
            case XCB_BUTTON_PRESS: {
                xcb_button_press_event_t *bp = (xcb_button_press_event_t *)event;
                
                    printf ("Button %d pressed\n", bp->detail );

                mouse_but[0] = (char) 1;

                int *ptr5 = (int *)(&mouse_but[1]);
                *ptr5 = bp->detail;

                mouse_but[5] = (char) 1;
                send_input(mouse_kb->sockfd, mouse_but, 6); 
                memset(mouse_but, 0,6);
                
            }			
            default:
                printf ("Unknown event: %d\n",
                        event->response_type);
                break;
        }

    }  
       
}


XImage *create_ximage(Display *display, Visual *visual, int width, int height, char *image)
{
    return XCreateImage(display, visual, 24, ZPixmap, 0, image, width, height, 32, 0);
}

int putimage(char *image, Display *display, Visual *visual, Window window, GC gc)
{    
    XImage *ximage = create_ximage(display, visual, WIDTH, HEIGHT, image);
    XEvent event;
    bool exit = false;
    int r;
    r = XPutImage(display, window, gc, ximage, 0, 0, 0, 0, WIDTH, HEIGHT);
    if (r) {
        print("Error occurred while displaying X11 image (%d)", r);
    }
    return 0;
}


void *func(void *args) 
{
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
            fprintf(stderr, "Error reading from socket.\n");
            exit(1);
            // should make it exit gracefully but problem for future
        }
    } 
} 

int main(int argc, char *argv[]) 
{   
    
    if (argc != 3){
        printf( "Invalid argument count"
                "Usage: %s <ip> <port>", argv[0]);
        exit(1);
    }
    else
    {
        
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

        int sockfd, connfd; 
        struct sockaddr_in servaddr, cli;

        int sockfd2, connfd2; 
        struct sockaddr_in servaddr2, cli2; 
        int flags = 1;
        
        sockfd = rsocket(AF_INET, SOCK_STREAM, 0);
        printf("Created RSocket FD: %d\n", sockfd);

        if (sockfd == -1) { 
            printf("Could not create RDMA socket.\n"); 
            exit(1); 
        } 
        else
            printf("Created RDMA socket.\n");
        
        sockfd2 = socket(AF_INET, SOCK_STREAM, 0); 
        setsockopt(sockfd2, 6, TCP_NODELAY, flags, sizeof(flags));
    
        if (sockfd2 == -1) { 
            printf("Could not create TCP socket.\n"); 
            exit(1); 
        } 
        else
            printf("Created TCP socket.\n"); 

        /* Attempt to connect to the server */

        printf("Trying to connect to %s:%d (RDMA)\n", argv[1], atoi(argv[2]));
        
        memset(&servaddr,0, sizeof(servaddr)); 
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
        servaddr.sin_port = htons(atoi(argv[2])); 

        memset(&servaddr2, 0,sizeof(servaddr2)); 
        servaddr2.sin_family = AF_INET; 
        servaddr2.sin_addr.s_addr = inet_addr(argv[1]); 
        servaddr2.sin_port = htons(atoi(argv[2]) + 1);   

        if (rconnect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
            printf("RDMA connection with the server failed.\n"); 
            exit(1); 
        }

        printf("Trying to connect to %s:%d (TCP)\n", argv[1], atoi(argv[2]) + 1);

        if (connect(sockfd2, (SA*)&servaddr2, sizeof(servaddr2)) != 0) { 
            printf("TCP connection with the server failed.\n"); 
            exit(1); 
        } 

        printf("TCP connected to the server..\n"); 
        pthread_t thread1, thread2;
        int th1, th2;

        /* Pass X11 and XCB configuration information to the client threads */

        mouse_param *mouse_config = calloc(1, sizeof(mouse_param));
        screen_param* screen_config = calloc(1, sizeof(screen_param));

        mouse_config->c = c;
        //mouse_config->screen = screen2;
        //mouse_config->xcb_window = xcb_window;
        mouse_config->x11_window = window;
        mouse_config->sockfd = sockfd2;
        mouse_config->connfd = connfd2;
        mouse_config->display = display;

        screen_config->sockfd = sockfd;
        screen_config->display = display;
        screen_config->visual = visual;
        screen_config->x11_window = window;
        screen_config->gc = gc;

        th1 = pthread_create(&thread1, NULL, capture_mouse_keyboard, (void*) mouse_config);
        th2 = pthread_create(&thread2, NULL, func, (void*) screen_config);

        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);

        printf("All threads have exited.\n");

        exit(0);
    }
}