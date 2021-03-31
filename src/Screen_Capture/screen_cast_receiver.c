/* Telescope Client */

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
#include <sys/time.h>

/* todo: fix! dynamic width and height */
/* todo: fix! errors should be printed to stderr, see draw_display for example */
#define MAX 10000000 // should allocate just what is needed (frame size)
#define SA struct sockaddr 
#define WIDTH 1920 // make dynamic
#define HEIGHT 1080 // make dynamic


#define RATE "Mbit/s"
#define RATE_FRAC 131072

XImage *create_ximage(Display *display, Visual *visual, int width, int height, char *image)
{
    return XCreateImage(display, visual, 24,ZPixmap, 0, image, width, height, 32, 0);
}

int putimage(char *image ,Display *display,Visual *visual,Window *window,GC gc)
{    XImage *ximage = create_ximage(display, visual, WIDTH, HEIGHT, image);
        XEvent event;
        bool exit = false;
        int r;
        /* todo: can you use xshm */
        r = XPutImage(display, *window,gc, ximage, 0, 0, 0, 0, WIDTH, HEIGHT );
        return 0;
}

void func(int sockfd, Display* display, Visual* visual, Window* window, GC gc) 
{
    char *buff = calloc(MAX,1); 
    int n; 
    ssize_t length, framebytes;
    int maxbytes = WIDTH*HEIGHT*4; // todo: make dynamic
    
    clock_t t1, t2, t3 = 0;
    double fps, tdiff, data_rate, put_time = 0;

    t1 = clock();
    
    while (1)
    {
        /* Read data from the RDMA buffer */
        length = rread(sockfd, buff+framebytes, maxbytes-framebytes);
        framebytes += length; 
        
        if (framebytes == maxbytes)
        {
            t2 = clock();
            putimage(buff, display, visual, window, gc);
            t3 = clock();

            printf("%f", (double) t3 - t1);

            /* Seconds required to put the image on the screen */
            put_time = (double) (t3 - t2) / (double) CLOCKS_PER_SEC;
            /* Seconds required to receive and put the image on the screen */
            tdiff = (double) (t3 - t1) / (double) CLOCKS_PER_SEC;
            /* Effective data rate including draw time */
            data_rate = ((double) framebytes / tdiff / RATE_FRAC);
            /* Effective framerate including receive time */
            fps = 1 / tdiff;
            /* Display statistics */
            printf("Time: %.4f, FPS: %.2f, Put FPS: %.2f, Rx: %.2f %s\n", tdiff, fps, 1/put_time, data_rate, RATE);
            
            /* Reset clock and buffer */
            t1 = clock();
            framebytes = 0;
        }
        else if (framebytes > maxbytes)
        {
            fprintf(stderr, "Critical: Invalid number of bytes read from stream\n");
            exit(1);
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
            printf("socket creation failed...\n"); 
            exit(0); 
        } 
        else
            printf("Socket successfully created..\n"); 
        memset(&servaddr,0, sizeof(servaddr)); 
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
        servaddr.sin_port = htons(atoi(argv[2])); 
        if (rconnect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
            printf("connection with the server failed...\n"); 
            exit(0); 
        } 
        else
            printf("connected to the server..\n"); 
        func(sockfd, display,visual,&window,gc); 
        rclose(sockfd); 
    }
} 
