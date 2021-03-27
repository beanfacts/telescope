
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

#define MAX 10000000 
#define PORT 6969  
#define SA struct sockaddr 
#define WIDTH 960
#define HEIGHT 540

XImage *create_ximage(Display *display, Visual *visual, int width, int height, char *image)
{
    return XCreateImage(display, visual, 24,ZPixmap, 0, image, width, height, 32, 0);
}

int putimage(char *image ,Display *display,Visual *visual,Window *window,GC gc)
{    XImage *ximage = create_ximage(display, visual, WIDTH, HEIGHT, image);
        XEvent event;
        bool exit = false;
        int r;
            r = XPutImage(display, *window,gc, ximage, 0, 0, 0, 0, WIDTH, HEIGHT );
            printf("RES: %i\n", r);
        return 0;
}

void func(int sockfd , Display *display,Visual *visual,Window *window,GC gc) 
{
    char *buff=calloc(MAX,1); 
    int n; 
    int length;
    int framebytes = 0;
    int maxbytes = WIDTH*HEIGHT*4;
    while (1) {
        length = rread(sockfd, buff+framebytes , maxbytes-framebytes);
        framebytes = framebytes + length; 
        if (framebytes >= maxbytes){
            printf(" %d",framebytes);
            framebytes = 0;
            putimage(buff,display,visual,window,gc);
            memset(buff,0, MAX); 
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
