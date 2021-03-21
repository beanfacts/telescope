
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX 10000000 
#define PORT 6969 
#define SA struct sockaddr 

XImage *create_ximage(Display *display, Visual *visual, int width, int height, char *image)
{
    return XCreateImage(display, visual, 24,ZPixmap, 0, image, width, height, 32, 0);
}

int putimage(char *image ,Display *display,Visual *visual,Window window,GC gc)
{    XImage *ximage = create_ximage(display, visual, 960, 540, image);
        XEvent event;
        bool exit = false;
        int r;
            r = XPutImage(display, window,gc, ximage, 0, 0, 0, 0, 960, 540 );
            printf("RES: %i\n", r);
        return 0;
}

void func(int sockfd , Display *display,Visual *visual,Window *window,GC gc) 
{
    char *buff=calloc(MAX,1); 
    int n; 
    int length;
    int framebytes = 0;
    int maxbytes = 960*540*4;
    while (1) {
        length = read(sockfd, buff+framebytes , maxbytes-framebytes);
        framebytes = framebytes + length; 
        if (framebytes >= maxbytes){
            printf(" %d",framebytes);
            framebytes = 0;
            putimage(buff,display,visual,window,gc);
            bzero(buff, MAX); 
        }
        if (length){
            printf("%d\n", framebytes); 
        }
    } 
} 

int main() 
{   
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
    window = XCreateSimpleWindow(display,DefaultRootWindow(display),0, 0, 960, 540, 0,win_b_color, win_w_color);
    visual = DefaultVisual(display, 0);
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);
    XFlush(display);
    gc = XCreateGC(display, window, 0, NULL);
    int sockfd, connfd; 
    struct sockaddr_in servaddr, cli; 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(PORT); 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
        printf("connected to the server..\n"); 
    func(sockfd, display,visual,window,gc); 
    close(sockfd); 
} 
