#include <X11/Xlib.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h>
#include <sys/socket.h> 
#include <unistd.h>
#include <arpa/inet.h> 
#include <netinet/tcp.h>
#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 

void func(int sockfd, char *buff, int size) 
{ 
    //char buff[MAX]; 
	//char * buff = calloc(MAX,sizeof(char));
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
  
int main(int argc, char *argv[]) 
{ 
    int sockfd, connfd; 
    struct sockaddr_in servaddr, cli; 
    int flags = 1;
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    setsockopt(sockfd,6,TCP_NODELAY, flags, sizeof(flags));
    
    if (argc < 2) {
        printf("Please specify ip address argument\n");
        exit(1);
    }

    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
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
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
        printf("connected to the server..\n"); 



    Display *display;
    Window window;
    XEvent event;
    int s;

    char *mouse_mov = calloc(9,sizeof(char));
    char *mouse_but = calloc(6,sizeof(char));
    char *keyboard = calloc(6, sizeof(char));
    XWindowAttributes win_attri;

 
      int fd;
	Display *dpy;
	Window root, child;
	int rootX, rootY, winX, winY;
	unsigned int mask;
int flag = 0;
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
 
    s = DefaultScreen(display);
 

    window = XCreateSimpleWindow(display, RootWindow(display, s), 10, 10, 1280, 720, 1,
                           BlackPixel(display, s), WhitePixel(display, s));
 

    XSelectInput(display, window, KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask);

    XMapWindow(display, window);
    
    int pre_xposi = 0;
    int pre_yposi = 0;
    int new_xposi = 0;
    int new_yposi = 0;

  
    
    while (1)
    {
        
        XNextEvent(display, &event);
        
        if (event.type == KeyPress)
        {

            keyboard[0] = (char) 2;

            int *ptr = (int *)(&keyboard[1]);
            *ptr =  event.xkey.keycode;

            keyboard[5] = (char) 1;


            //printf("sending code = %d\n", keyboard[0]);
            //printf("keycode = %d\n", *ptr);
            //printf("press or not? %d\n", keyboard[5]);


            //code for sending keyboard array is here
            func(sockfd, keyboard,6); 


            
        }
        else if (event.type == KeyRelease)
        {
            
            
            keyboard[0] = (char) 2;

            int *ptr1 = (int *)(&keyboard[1]);
            *ptr1 =  event.xkey.keycode;

            keyboard[5] = (char) 0;
        
/*
            printf("sending code = %d\n", keyboard[0]);
            printf("keycode = %d\n", *ptr1);
            printf("press or not? %d\n", keyboard[5]);
*/

            //code for sending keyboard array is here
            func(sockfd,keyboard, 6);


            
        }
        else if (XQueryPointer(display,window,&root,&child,
              &rootX,&rootY,&winX,&winY,&mask) && event.type != ButtonPress && event.type != ButtonRelease)
        {

            /*
            if (flag) {
                printf("flag ignore --------- \n");
                flag = 0;
                continue;
            }
            */
            
            XQueryPointer(display,window,&root,&child,
              &rootX,&rootY,&winX,&winY,&mask);
            
	        XGetWindowAttributes(display, window, &win_attri);
                
            int width = win_attri.width;
            int height = win_attri.height;

            int cen_x = width/2;
            int cen_y = height/2;

            
            new_xposi = winX;
            new_yposi = winY;

            int deltax = new_xposi-pre_xposi;
            int deltay = new_yposi - pre_yposi;

            pre_xposi = winX;
            pre_yposi = winY;
            

           //int deltax = winX - 100;
           //int deltay = winY - 100;

            mouse_mov[0] = (char) 0;

            if ((deltax != 0 || deltay != 0)&&(flag == 0)) {

                
            flag = 1;
            printf("delta x: %d, delta y: %d\n", deltax, deltay);
            int *ptr2 = (int *)(&mouse_mov[1]);
            *ptr2 = deltax;

            int *ptr3 = (int *)(&mouse_mov[5]);
            *ptr3 = deltay; 
            func(sockfd,mouse_mov, 16); 

            }
            else if (flag == 1){
                flag = 0;
            }
            XWarpPointer(display, 0, window, 0, 0, 0, 0, cen_x, cen_y);
            //flag = 1;

            
            //XWarpPointer(display, window, window, NULL, NULL, NULL, NULL, cen_x, cen_y);

            //count ++;
            //XWarpPointer(display, None, DefaultRootWindow(display), 0, 0, 0, 0, cen_x, cen_y);
                                        //printf("moved to center\n");
                    
            
            
    /*
            printf("sending type is %d \n", mouse_mov[0]);
            printf("\tdx = %d\n", *ptr2);
            printf("\tdy = %d\n", *ptr3);
    */
            //code for sending mouse movement
                        
        

/*
	        XGetWindowAttributes(display, window, &win_attri);
                
            int width = win_attri.width;
            int height = win_attri.height;

            int cen_x = width/2;
            int cen_y = height/2;

            //printf("%d\n",XWarpPointer(display, window, window, 0, 0, cen_x-300, height, cen_x, cen_y));


            XWarpPointer(display, window, window, 0, 0, cen_x-300, height, cen_x, cen_y);
            XWarpPointer(display, window, window, 0, 0, width, cen_y-150, cen_x, cen_y);
            XWarpPointer(display, window, window, 0, cen_y+150, width, height, cen_x, cen_y);
            XWarpPointer(display, window, window, cen_x+300, 0, width, height, cen_x, cen_y);


            if((winX == cen_x-300)||(winX == cen_x+300)||(winY == cen_y - 150)||(winY == cen_y + 150)){

                pre_yposi = cen_y;
                pre_xposi = cen_x;
                new_xposi = cen_x;
                new_yposi = cen_y;
                flag = 1;
                //new_yposi = cen_y;
                //new_xposi = cen_x;
                printf("moved to center\n");
            
                
            }
            else{
                flag = 0;
                new_xposi = winX;
                new_yposi = winY;

                int deltax = new_xposi-pre_xposi;
                int deltay = pre_yposi-new_yposi;


                pre_xposi = winX;
                pre_yposi = winY;

                
                mouse_mov[0] = (char) 0;

                printf("win x: %d, win y: %d\n", deltax, deltay);
                //XWarpPointer(display, window, window, NULL, NULL, NULL, NULL, cen_x, cen_y);

                
                //XWarpPointer(display, None, DefaultRootWindow(display), 0, 0, 0, 0, cen_x, cen_y);
                                            //printf("moved to center\n");
                        

                int *ptr2 = (int *)(&mouse_mov[1]);
                *ptr2 = deltax;

                int *ptr3 = (int *)(&mouse_mov[5]);
                *ptr3 = deltay; 
                
                printf("sending type is %d \n", mouse_mov[0]);
                printf("\tdx = %d\n", *ptr2);
                printf("\tdy = %d\n", *ptr3);
                //code for sending mouse movement

                
                func(sockfd,mouse_mov,9); 
                
            };
*/
         

            
        }

        else if (event.type == ButtonPress){
            if (event.xbutton.button == 1){
                printf("left button clicked\n");

                mouse_but[0] = (char) 1;

                int *ptr4 = (int *)(&mouse_but[1]);
                *ptr4 = event.xbutton.button;

                mouse_but[5] = (char) 1;

/*
                printf("sending type is %d \n", mouse_but[0]);
                printf("button code = %d\n",*ptr4);
                printf("press or not %d\n", mouse_but[5]);
*/

            // code for sending array of mouse button is here 
            func(sockfd,mouse_but, 6); 



            }
            else if (event.xbutton.button == 2){
                //printf("scroll button clicked\n");
                mouse_but[0] = (char) 1;

                int *ptr5 = (int *)(&mouse_but[1]);
                *ptr5 = event.xbutton.button;

                mouse_but[5] = (char) 1;

                /*
                printf("sending type is %d \n", mouse_but[0]);
                printf("button code = %d\n",*ptr5);
                printf("press or not %d\n", mouse_but[5]);
*/

            // code for sending array of mouse button is here 
            func(sockfd, mouse_but, 6); 
            }
            else if (event.xbutton.button == 3){
                //printf("right button clicked\n");
                mouse_but[0] = (char) 1;

                int *ptr6 = (int *)(&mouse_but[1]);
                *ptr6 = event.xbutton.button;

                mouse_but[5] = (char) 1;

                /*
                printf("sending type is %d \n", mouse_but[0]);
                printf("button code = %d\n",*ptr6);
                printf("press or not %d\n", mouse_but[5]);
*/

            // code for sending array of mouse button is here 
            func(sockfd, mouse_but, 6); 
            }
            else if (event.xbutton.button == 4){
                //printf("scroll up\n");
                mouse_but[0] = (char) 1;

                int *ptr7 = (int *)(&mouse_but[1]);
                *ptr7 = event.xbutton.button;

                mouse_but[5] = (char) 1;

                /*
                printf("sending type is %d \n", mouse_but[0]);
                printf("button code = %d\n",*ptr7);
                printf("press or not %d\n", mouse_but[5]);
*/

            // code for sending array of mouse button is here 
            func(sockfd,mouse_but, 6); 
            }
            else if (event.xbutton.button == 5){
                //printf("scroll down\n");
                mouse_but[0] = (char) 1;

                int *ptr8 = (int *)(&mouse_but[1]);
                *ptr8 = event.xbutton.button;

                mouse_but[5] = (char) 1;
                /*
                printf("sending type is %d \n", mouse_but[0]);
                printf("button code = %d\n",*ptr8);
                printf("press or not %d\n", mouse_but[5]);
*/

            // code for sending array of mouse button is here 
            func(sockfd, mouse_but, 6); 
            }
            
            else{
                //printf("some button on mouse pressed");
                mouse_but[0] = (char) 1;

                int *ptr9 = (int *)(&mouse_but[1]);
                *ptr9 = event.xbutton.button;

                mouse_but[5] = (char) 1;
                /*
                printf("sending type is %d \n", mouse_but[0]);
                printf("button code = %d\n",*ptr9);
                printf("press or not %d\n", mouse_but[5]);
*/

            // code for sending array of mouse button is here 
            func(sockfd, mouse_but, 6); 
            }


        }

        else if (event.type == ButtonRelease){
            //printf("button released\n");
            mouse_but[0] = (char) 1;

            int *ptr10 = (int *)(&mouse_but[1]);
            *ptr10 = event.xbutton.button;

            mouse_but[5] = (char) 0;
            /*
                printf("sending type is %d \n", mouse_but[0]);
                printf("button code = %d\n",*ptr10);
                printf("press or not %d\n", mouse_but[5]);
*/

            // code for sending array of mouse button is here 
            func(sockfd, mouse_but, 6); 

        }
        
        memset(mouse_mov, 0,9);
        memset(mouse_but,0 ,6);
        memset(keyboard, 0,6);
        
        
    }

    XCloseDisplay(display);
    
  
    // close the socket 
    close(sockfd); 
} 
