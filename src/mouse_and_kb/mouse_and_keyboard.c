#include <X11/Xlib.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <arpa/inet.h> 
#include <pthread.h>
#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 



void func(int sockfd, char *buff, int size) 
{ 
    //char buff[MAX]; 
	//char * buff = calloc(MAX,sizeof(char));
    //for(int i=0; i < size; i++){
    //    printf("%hhx", *(buff+i));
    //}
    //printf("\n");
    int n; 
    //for (;;) { 
        //printf("%d", *buff);
        //bzero(buff, size); 
        //printf("Enter the string : "); 
        n = 0; 
        // while ((buff[n++] = getchar()) != '\n') 
        //     ; 
        write(sockfd, buff, size); 
        bzero(buff, size); 
        read(sockfd, buff, size); 
        //printf("From Server : %s \n", buff); 
        // if ((strncmp(buff, "exit", 4)) == 0) { 
        //     printf("Client Exit...\n"); 
        //     break; 
        // } 
    //} 
} 

void *mouse_move(Display *display,Window window,Window root,Window child,int rootX,int rootY,int winX,int winY,unsigned int mask,XWindowAttributes win_attri,int new_xposi,int new_yposi,int pre_xposi,int pre_yposi,int sockfd,int flag, int *ret_newx, int *ret_newy, int *ret_prex, int *ret_prey, int *ret_flag){
    XQueryPointer(display,window,&root,&child,
              &rootX,&rootY,&winX,&winY,&mask);
            char *mouse_mov = malloc(9);
	        XGetWindowAttributes(display, window, &win_attri);
                
            int width = win_attri.width;
            int height = win_attri.height;

            int cen_x = width/2;
            int cen_y = height/2;

            
            new_xposi = winX;
            new_yposi = winY;

            int deltax = new_xposi-pre_xposi;
            int deltay = pre_yposi-new_yposi;

            pre_xposi = winX;
            pre_yposi = winY;
            

           //int deltax = winX - 100;
           //int deltay = winY - 100;

            mouse_mov[0] = (char) 0;

            if ((deltax != 0 || deltay != 0)&&(flag == 0)) {

                
            *ret_flag = 1;
            printf("delta x: %d, delta y: %d\n", deltax, deltay);
            //printf("new x: %d, new y: %d\n", new_xposi, new_yposi);
            //printf("pre x: %d, pre y: %d\n", pre_xposi, pre_yposi);
            
            int *ptr2 = (int *)(&mouse_mov[1]);
            *ptr2 = deltax;

            int *ptr3 = (int *)(&mouse_mov[5]);
            *ptr3 = deltay; 
            func(sockfd,mouse_mov,9); 

            }
            else{
                *ret_flag = 0;
            }
            XWarpPointer(display, 0, window, 0, 0, 0, 0, cen_x, cen_y);
            *ret_newx = new_xposi;
            *ret_newy = new_yposi;
            *ret_prex = pre_xposi;
            *ret_prey = pre_yposi;

            //return (void *)pre_yposi, (void *)pre_xposi, (void *)new_yposi, (void *)new_xposi;
            //pthread_exit(NULL);

}
  
int main() 
{ 
    int sockfd, connfd; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
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

    char *mouse_mov = malloc(9);
    char *mouse_but = malloc(6);
    char *keyboard = malloc(6);
    XWindowAttributes win_attri;

 

    display = XOpenDisplay(NULL);
    
      int fd;
	Display *dpy;
	Window root, child;
	int rootX, rootY, winX, winY;
	unsigned int mask;


	//printf("root x: %d \n", rootX);

    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
 
    s = DefaultScreen(display);
 
    
    window = XCreateSimpleWindow(display, RootWindow(display, s), 10, 10, 200, 200, 1,
                           BlackPixel(display, s), WhitePixel(display, s));
 
 	XQueryPointer(display,window,&root,&child,
              &rootX,&rootY,&winX,&winY,&mask);
    XSelectInput(display, window, KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask);

    XMapWindow(display, window);
    
    int pre_xposi = 0;
    int pre_yposi = 0;
    int new_xposi = 0;
    int new_yposi = 0;



    //int count = 0;

    int flag = 0;
    

  
    
    while (1)
    {
        
        XNextEvent(display, &event);
        
        //pthread_t thread_id;
        //pthread_create(&thread_id, NULL, mouse_move(display, window, root, child, rootX, rootY, winX, winY, mask, win_attri, new_xposi, new_yposi, pre_xposi, pre_yposi, sockfd, flag, &new_xposi, &new_yposi, &pre_xposi, &pre_yposi, &flag), NULL);
        mouse_move(display, window, root, child, rootX, rootY, winX, winY, mask, win_attri, new_xposi, new_yposi, pre_xposi, pre_yposi, sockfd, flag, &new_xposi, &new_yposi, &pre_xposi, &pre_yposi, &flag);
        //pthread_exit(NULL);

        
        /*
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
            int deltay = pre_yposi-new_yposi;

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
            func(sockfd,mouse_mov,9); 

            }
            else{
                flag = 0;
            }
            XWarpPointer(display, 0, window, 0, 0, 0, 0, cen_x, cen_y);
            */
            




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
        /*
        else if (XQueryPointer(display,window,&root,&child,
              &rootX,&rootY,&winX,&winY,&mask) != 0)
        {
                       printf("no move\n");        
        }
        */

        else if (event.type == ButtonPress){
            if (event.xbutton.button == 1){
                //printf("left button clicked\n");

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
        
        bzero(mouse_mov, 9);
        bzero(mouse_but, 6);
        bzero(keyboard, 6);
        
        
    }

    XCloseDisplay(display);
 
  
    // function for chat 
    
  
    // close the socket 
    close(sockfd); 
} 
