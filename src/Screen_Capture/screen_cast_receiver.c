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
#include <arpa/inet.h>
#include <X11/Xutil.h>
#include <rdma/rsocket.h>
#include <sys/time.h>

/* todo: fix! dynamic width and height */
/* todo: fix! errors should be printed to stderr, see draw_display for example */
#define MAX 10000000 // should allocate just what is needed (frame size)
#define SA struct sockaddr 
#define WIDTH 960 // make dynamic
#define HEIGHT 540 // make dynamic
/* todo: fix! ugly function space!!!! */

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

double convert_timediff(struct timeval *t2, struct timeval *t1) {
    double ct1, ct2;
    ct1 = (double) t1->tv_sec + ((double) t1->tv_usec / (double) 1000000);
    ct2 = (double) t2->tv_sec + ((double) t2->tv_usec / (double) 1000000);
    return ct2 - ct1;
}

void func(int sockfd, Display* display, Visual* visual, Window* window, GC gc) 
{
    char *buff = calloc(MAX,1); 
    int n; 
    ssize_t length, framebytes;
    int maxbytes = WIDTH*HEIGHT*4; // todo: make dynamic
    
    //clock_t t1, t2, t3 = 0;
    struct timeval t1, t2, t3;
    double fps, tdiff, data_rate, put_time = 0;

    gettimeofday(&t1, NULL);
    
    while (1)
    {
        /* Read data from the RDMA buffer */
        length = rread(sockfd, buff+framebytes, maxbytes-framebytes);
        framebytes += length; 
        
        if (framebytes == maxbytes)
        {
            gettimeofday(&t2, NULL);
            putimage(buff, display, visual, window, gc);
            gettimeofday(&t3, NULL);

            /* Seconds required to put the image on the screen */
            //put_time = (double) (t3 - t2) / (double) CLOCKS_PER_SEC;
            put_time = convert_timediff(&t3, &t2);
            /* Seconds required to receive and put the image on the screen */
            tdiff = convert_timediff(&t3, &t1);
            /* Effective data rate including draw time */
            data_rate = ((double) framebytes / tdiff / (double) RATE_FRAC);
            /* Effective framerate including receive time */
            fps = 1 / tdiff;
            /* Display statistics */
            printf("Time: %.4f, FPS: %.2f, Put FPS: %.2f, Rx: %.2f %s\n", tdiff, fps, 1/put_time, data_rate, RATE);
            
            /* Reset clock and buffer */
            gettimeofday(&t1, NULL);
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
