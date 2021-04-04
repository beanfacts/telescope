
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <arpa/inet.h>
#include <X11/Xutil.h>
#include <rdma/rsocket.h>

#include <X11/Xlib-xcb.h>
#include <xcb/xproto.h>
#include <xcb/xfixes.h>
#include <xcb/shm.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <netinet/tcp.h>

#define MAX 10000000 
#define PORT 6969  
#define SA struct sockaddr 
#define WIDTH  1920// make dynamic
#define HEIGHT 1080 // make dynamic

XImage *create_ximage(Display *display, Visual *visual, int width, int height, char *image)
{
    return XCreateImage(display, visual, 24,ZPixmap, 0, image, width, height, 32, 0);
}

int putimage(char *image ,Display *display,Visual *visual,xcb_window_t *window,GC gc)
{    XImage *ximage = create_ximage(display, visual, WIDTH, HEIGHT, image);
        XEvent event;
        bool exit = false;
        int r;
            r = XPutImage(display, *window,gc, ximage, 0, 0, 0, 0, WIDTH, HEIGHT );
        return 0;
}

void get_center(xcb_connection_t *c, xcb_window_t window, int *ret_cenx, int *ret_ceny) {
    xcb_get_geometry_cookie_t cookie;
    xcb_get_geometry_reply_t *reply;

    cookie = xcb_get_geometry(c, window);
    
    if ((reply = xcb_get_geometry_reply(c, cookie, NULL))) {
		*ret_cenx = (reply->width)/2;
		*ret_ceny = (reply->height)/2;
        
    }
    free(reply);
}

static void setCursor (xcb_connection_t*, xcb_screen_t*, xcb_window_t, int);
static void testCookie(xcb_void_cookie_t, xcb_connection_t*, char *); 

static void testCookie (xcb_void_cookie_t   cookie,
                        xcb_connection_t    *connection,
                        char                *errMessage )
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
                        int               cursorId )
{
    xcb_font_t font = xcb_generate_id (connection);
    xcb_void_cookie_t fontCookie = xcb_open_font_checked (connection, font, strlen ("cursor"), "cursor");
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

void func2(int sockfd, char *buff, int size) 
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
    memset(buff, 0,size); 
    read(sockfd, buff, size); 
     
} 

void func(int sockfd , Display *display,Visual *visual,Window *window,GC gc) 
{
    char *buff=calloc(MAX,1); 
    int n; 
    int length;
    int framebytes = 0;
    int maxbytes = WIDTH*HEIGHT*4;

    xcb_connection_t *c = XGetXCBConnection(display);
    xcb_screen_t *screen   = xcb_setup_roots_iterator (xcb_get_setup (c)).data;             
    xcb_window_t window2    = xcb_generate_id (c);
    uint32_t     mask2      = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t     values[2]  = {screen->white_pixel,
                                    XCB_EVENT_MASK_POINTER_MOTION
                                    | XCB_EVENT_MASK_BUTTON_PRESS 
                                    | XCB_EVENT_MASK_BUTTON_RELEASE
                                    | XCB_EVENT_MASK_KEY_PRESS
                                    | XCB_EVENT_MASK_KEY_RELEASE
                                    | XCB_EVENT_MASK_ENTER_WINDOW
                                    | XCB_EVENT_MASK_LEAVE_WINDOW
                                    };
    xcb_create_window (c,    
                        0,                            
                        window2,                        
                        screen->root,                  
                        0, 0,                          
                        WIDTH, HEIGHT,                      
                        10,                            
                        XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                        screen->root_visual,           
                        mask2, values );         
          
    xcb_map_window (c, window2);
    sleep(2);
    xcb_unmap_window(c, *window);
    xcb_flush(c);

    int count = 0;
    int cenx;
    int ceny;
    xcb_generic_event_t *event;
    int pre_x = 0;
    int pre_y = 0;
    int new_x = 0;
    int new_y = 0;
    int deltax = 0;
    int deltay = 0;
    int flag = 0;
    char *mouse_mov = malloc(9);
    char *mouse_but = malloc(6);
    char *keyboard = malloc(6);

    while (1) {
        length = rread(sockfd, buff+framebytes , maxbytes-framebytes);
        framebytes = framebytes + length; 
        if (framebytes >= maxbytes){
            printf(" %d",framebytes);
            framebytes = 0;
            putimage(buff,display,visual,*window,gc);
            memset(buff,0, MAX); 
            
            switch (event->response_type & ~0x80)
            {
                case XCB_MOTION_NOTIFY:{
                    xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
                    get_center(c, window2, &cenx, &ceny);
                    new_x = motion->event_x;	
                    new_y = motion->event_y;

                    deltax = new_x - pre_x;
                    deltay = new_y - pre_y;

                    pre_x = motion->event_x;
                    pre_y = motion->event_y;
                    
                    if (((deltax!=0) && (deltay != 0))&&(flag == 0)){
                        flag = 1;
                        printf("(%d : %d)\n", deltax, deltay);
                        mouse_mov[0] = (char) 0;
                        int *ptr2 = (int *)(&mouse_mov[1]);
                        *ptr2 = deltax;
                        int *ptr3 = (int *)(&mouse_mov[5]);
                        *ptr3 = deltay; 
                        func2(sockfd,mouse_mov,9); 
                        bzero(mouse_mov, 9);
                    }
                    else if(flag == 1){
                        flag = 0;
                    }
                    xcb_warp_pointer(c, screen->root, window2, 0,0,0,0, cenx, ceny);
                    break;	
                }
                case XCB_BUTTON_RELEASE: {
                    xcb_button_release_event_t *br = (xcb_button_release_event_t *)event;
                    printf ("Button %d released\n", br->detail);
                    mouse_but[0] = (char) 1;
                    int *ptr6 = (int *)(&mouse_but[1]);
                    *ptr6 = br->detail;
                    mouse_but[5] = (char) 0;
                    func2(sockfd, mouse_but, 6); 
                    bzero(mouse_but, 6);
                    break;
                }
                case XCB_KEY_PRESS: {
                    xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;
                    printf ("Key pressed in window %d\n", kp->detail);
                    keyboard[0] = (char) 2;
                    int *ptr = (int *)(&keyboard[1]);
                    *ptr =  kp->detail;
                    keyboard[5] = (char) 1;
                    func2(sockfd,keyboard, 6);
                    break;
                }
                case XCB_KEY_RELEASE: {
                    xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
                    printf ("Key released in window %d\n", kr->detail);
                    keyboard[0] = (char) 2;
                    int *ptr = (int *)(&keyboard[1]);
                    *ptr =  kr->detail;
                    keyboard[5] = (char) 0;
                    func2(sockfd,keyboard, 6);
                    bzero(keyboard, 6);
                    break;
                }
                case XCB_ENTER_NOTIFY:{
                    xcb_xfixes_hide_cursor(c, screen->root);
                    //printf("event type: %d", event->response_type);
                    printf("enter\n");
                    break;
                }
                case XCB_LEAVE_NOTIFY:{
                    xcb_xfixes_show_cursor(c, screen->root);
                    setCursor (c, screen, window2, 58);
                    //printf("event type: %d", event->response_type);
                    printf("leave\n");
                    break;
                }
                case XCB_BUTTON_PRESS: {
                    xcb_button_press_event_t *bp = (xcb_button_press_event_t *)event;
                    //printf("event type: %d", event->response_type);
                    printf ("Button %d pressed\n", bp->detail );
                    mouse_but[0] = (char) 1;
                    int *ptr5 = (int *)(&mouse_but[1]);
                    *ptr5 = bp->detail;
                    mouse_but[5] = (char) 1;
                    func2(sockfd, mouse_but, 6); 
                    bzero(mouse_but, 6);
                    break;
                }			
                default:{
                    fprintf (stderr, "Unknown event: %d\n", event->response_type);
                    break;
                }
            }
        }
        if (length){ 
            printf("%d\n", framebytes); 
        }
    }
}

int main(int argc, char *argv[]) 
{   
    if (argc != 3){
        printf("Please specify the IP address and port");
        exit(1);
    }
    else{
        int win_b_color;
        int win_w_color;
        XEvent xev;
        Window window;
        GC gc;
        Display *display = XOpenDisplay(NULL);
        Visual *visual;
        XImage *ximage;
        win_b_color = BlackPixel(display, DefaultScreen(display));
        win_w_color = BlackPixel(display, DefaultScreen(display));
        window = XCreateSimpleWindow(display,DefaultRootWindow(display),0, 0, WIDTH, HEIGHT, 0,win_b_color, win_w_color);
        visual = DefaultVisual(display, 0);
        Atom window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
        long value = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
        XChangeProperty(display, window, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
        XSelectInput(display, window, ExposureMask | KeyPressMask);
        XMapWindow(display, window);
        XFlush(display);
        gc = XCreateGC(display, window, 0, NULL);
        int sockfd, connfd; 
        struct sockaddr_in servaddr, cli; 
        sockfd = rsocket(AF_INET, SOCK_STREAM, 0); 
        if (sockfd == -1) { 
            fprintf(stderr, "socket creation failed...\n"); 
            exit(0); 
        } 
        else
            printf("Socket successfully created..\n"); 
        memset(&servaddr,0, sizeof(servaddr)); 
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
        servaddr.sin_port = htons(atoi(argv[2])); 
        if (rconnect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
            fprintf(stderr, "connection with the server failed...\n"); 
            exit(0); 
        } 
        else{
            int tsockfd, tconnfd; 
            struct sockaddr_in servaddr, cli; 
            int flags = 1;
            sockfd = socket(AF_INET, SOCK_STREAM, 0); 
            setsockopt(sockfd,6,TCP_NODELAY, flags, sizeof(flags));
            
            if (argc < 2) {
                fprintf(stderr, "please specify ip address argument.\n");
                exit(1);
            }

            if (tsockfd == -1) { 
                fprintf(stderr, "socket creation failed...\n"); 
                exit(0); 
            }
            else
                printf("Socket successfully created..\n"); 
            memset(&servaddr, 0,sizeof(servaddr)); 

            printf("Trying to connect to %s\n", argv[1]);
            // assign IP, PORT
            servaddr.sin_family = AF_INET; 
            servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
            servaddr.sin_port = htons(PORT);
            // connect the client socket to server socket
            if (connect(tsockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                fprintf(stderr, "connection with the server failed...\n");
                exit(0);
            } 
            else{
                printf("connected to the server..\n");    
                func(tsockfd, display,visual,&window,gc); 
                rclose(tsockfd);
            }
        }
    }
} 
