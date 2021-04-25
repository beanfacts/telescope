  
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <xcb/xproto.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>
#include <netinet/tcp.h>
#include <string.h> 
#include <netdb.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h> 
#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 
#include <time.h>

#define THREAD_NUM 8
typedef struct Task {
    int sockfd;
    xcb_connection_t *connection ;
    xcb_screen_t *screen;
    xcb_window_t window;
    uint32_t     mask ;
    //uint32_t     values[2];
    int cenx;
    int ceny;
    xcb_generic_event_t *event;
    int pre_x;
    int pre_y;
    int new_x;
    int new_y;
    int deltax;
    int deltay;
    int flag;

} Task;

Task taskQueue[256];
int taskCount = 0;

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;

static void setCursor (xcb_connection_t*, xcb_screen_t*, xcb_window_t, int);
static void testCookie(xcb_void_cookie_t, xcb_connection_t*, char *); 
void* startThread1(void* args) ;
void executeMovement(Task * task);
void executeDeviceAction(Task *task);
    static void
    testCookie (xcb_void_cookie_t cookie,
                xcb_connection_t *connection,
                char *errMessage )
    {   
        xcb_generic_error_t *error = xcb_request_check (connection, cookie);
        if (error) {
            fprintf (stderr, "ERROR: %s : %"PRIu8"\n", errMessage , error->error_code);
            xcb_disconnect (connection);
            exit (-1);
        }   
    }   



    static void
    setCursor (xcb_connection_t *connection,
                xcb_screen_t     *screen,
                xcb_window_t      window,
                int               cursorId )
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

    cookie = xcb_get_geometry(c, window);
    
    if ((reply = xcb_get_geometry_reply(c, cookie, NULL))) {
		*ret_cenx = (reply->width)/2;
		*ret_ceny = (reply->height)/2;
        
    }
    free(reply);
}

// void executeTask(Task* task) {
//     //usleep(50000);
//     int result = task->a + task->b;
//     printf("The sum of %d and %d is %d\n", task->a, task->b, result);
// }

void submitTask(Task task) {
    pthread_mutex_lock(&mutexQueue);
    taskQueue[taskCount] = task;
    taskCount++;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}

// 1 2 3 4 5
// 2 3 4 5

void* startThread1(void* args) {
    while (1) {
        Task task;

        pthread_mutex_lock(&mutexQueue);
        while (taskCount == 0) {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }

        task = taskQueue[0];
        int i;
        for (i = 0; i < taskCount - 1; i++) {
            taskQueue[i] = taskQueue[i + 1];
        }
        taskCount--;
        pthread_mutex_unlock(&mutexQueue);
        //executeTask(&task);
        executeMovement(&task);
    }
}


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
  
void executeMovement(Task *task){

        // int sockfd = task->sockfd;
        // xcb_connection_t *connection = task->connection;
     
        // xcb_screen_t *screen = task->screen;

        // xcb_window_t window    = task->window;

        // uint32_t     mask      = task->mask;
        // uint32_t     values[2] ={screen->white_pixel,
        //                              XCB_EVENT_MASK_POINTER_MOTION
		// 							 | XCB_EVENT_MASK_BUTTON_PRESS 
		// 							 | XCB_EVENT_MASK_BUTTON_RELEASE
		// 							 | XCB_EVENT_MASK_KEY_PRESS
		// 							 | XCB_EVENT_MASK_KEY_RELEASE
        //                              | XCB_EVENT_MASK_ENTER_WINDOW
        //                              | XCB_EVENT_MASK_LEAVE_WINDOW
        //                              };

        // xcb_create_window (connection,    
        //                    0,                            
        //                    window,                        
        //                    screen->root,                  
        //                    0, 0,                          
        //                    300, 300,                      
        //                    10,                            
        //                    XCB_WINDOW_CLASS_INPUT_OUTPUT, 
        //                    screen->root_visual,           
        //                    mask, values );               

        // xcb_map_window (connection, window);

        // xcb_flush (connection);
		// int cenx =task->cenx;
		// int ceny = task->ceny;
		

        // xcb_generic_event_t *event;

		


		// int pre_x = task->pre_x;
		// int pre_y = task->pre_x;
		// int new_x = task->new_x;
		// int new_y = task->new_y;
		// int deltax = task->deltax;
		// int deltay = task->deltay;



        char *mouse_mov = malloc(9);
        char *mouse_but = malloc(6);
        char *keyboard = malloc(6);
        // xcb_xfixes_query_version(connection,4,0);
        // xcb_xfixes_show_cursor(connection, screen->root);
        // int flag = 0;


        while ( (task->event = xcb_wait_for_event (task->connection)) ) {
            
          
			switch (task->event->response_type & ~0x80)
			{
			case XCB_MOTION_NOTIFY:{
			xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)task->event;
			get_center(task->connection, task->window, &task->cenx, &task->ceny);
            task->new_x = motion->event_x;	
			task->new_y = motion->event_y;

			task->deltax = task->new_x - task->pre_x;
			task->deltay = task->new_y - task->pre_y;

			task->pre_x = motion->event_x;
			task->pre_y = motion->event_y;
			
            if (((task->deltax!=0) && (task->deltay != 0))&&(task->flag == 0)){

            task->flag = 1;


            
                
            printf("(%d : %d)\n", task->deltax, task->deltay);

            mouse_mov[0] = (char) 0;


            int *ptr2 = (int *)(&mouse_mov[1]);
            *ptr2 = task->deltax;

            int *ptr3 = (int *)(&mouse_mov[5]);
            *ptr3 = task->deltay; 
            func(task->sockfd,mouse_mov,9); 
            bzero(mouse_mov, 9);
            }
            else if(task->flag == 1){
                task->flag = 0;
            }

            xcb_warp_pointer(task->connection, task->screen->root, task->window, 0,0,0,0, task->cenx, task->ceny); 
			break;
            	
			}

            case XCB_BUTTON_PRESS: {
				xcb_button_press_event_t *bp = (xcb_button_press_event_t *) task->event;
                
                    printf ("Button %d pressed\n", bp->detail );

                mouse_but[0] = (char) 1;

                int *ptr5 = (int *)(&mouse_but[1]);
                *ptr5 = bp->detail;

                mouse_but[5] = (char) 1;
                func(task->sockfd, mouse_but, 6); 
                bzero(mouse_but, 6);
                
			}

			case XCB_BUTTON_RELEASE: {
                xcb_button_release_event_t *br = (xcb_button_release_event_t *)task->event;
                

                printf ("Button %"PRIu8" released\n", br->detail);

                mouse_but[0] = (char) 1;

                int *ptr6 = (int *)(&mouse_but[1]);
                *ptr6 = br->detail;

                mouse_but[5] = (char) 0;
                func(task->sockfd, mouse_but, 6); 
                bzero(mouse_but, 6);

                break;
            }

			case XCB_KEY_PRESS: {
                xcb_key_press_event_t *kp = (xcb_key_press_event_t *) task->event;
                
                printf ("Key pressed in window %d\n", kp->detail);

                keyboard[0] = (char) 2;

                int *ptr = (int *)(&keyboard[1]);
                *ptr =  kp->detail;

                keyboard[5] = (char) 1;

                func(task->sockfd,keyboard, 6);
                bzero(keyboard, 6);

                break;
            }

			case XCB_KEY_RELEASE: {
                xcb_key_release_event_t *kr = (xcb_key_release_event_t *)task->event;
                

                printf ("Key released in window %d\n", kr->detail);

                keyboard[0] = (char) 2;

                int *ptr = (int *)(&keyboard[1]);
                *ptr =  kr->detail;

                keyboard[5] = (char) 0;

                func(task->sockfd,keyboard, 6);
                bzero(keyboard, 6);


                break;
            }

            case XCB_ENTER_NOTIFY:{
                xcb_xfixes_hide_cursor(task->connection, task->screen->root);
                printf("enter\n");
                break;
            }

            case XCB_LEAVE_NOTIFY:{
                xcb_xfixes_show_cursor(task->connection, task->screen->root);
                setCursor (task->connection, task->screen, task->window, 58);
                
                printf("leave\n");
                break;
            }
			
			default:
				printf ("Unknown event: %"PRIu8"\n",
                        task->event->response_type);
                break;
			}
        }
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
    { printf("Socket successfully created..\n"); 
    memset(&servaddr, 0,sizeof(servaddr)); 
  
    printf("Trying to connect to %s\n", argv[1]);

    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
    servaddr.sin_port = htons(PORT);   
  }
       
    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
    {
        printf("connected to the server..\n"); 
    }

    printf("hello");

    

        printf("hello");
        xcb_connection_t *connection = xcb_connect (NULL, NULL);

     
        xcb_screen_t *screen = xcb_setup_roots_iterator (xcb_get_setup (connection)).data;


        
        xcb_window_t window    = xcb_generate_id (connection);

        uint32_t     mask      = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        uint32_t     values[2] = {screen->white_pixel,
                                     XCB_EVENT_MASK_POINTER_MOTION
									 | XCB_EVENT_MASK_BUTTON_PRESS 
									 | XCB_EVENT_MASK_BUTTON_RELEASE
									 | XCB_EVENT_MASK_KEY_PRESS
									 | XCB_EVENT_MASK_KEY_RELEASE
                                     | XCB_EVENT_MASK_ENTER_WINDOW
                                     | XCB_EVENT_MASK_LEAVE_WINDOW
                                     };

        xcb_create_window (connection,    
                           0,                            
                           window,                        
                           screen->root,                  
                           0, 0,                          
                           300, 300,                      
                           10,                            
                           XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                           screen->root_visual,           
                           mask, values );               

        xcb_map_window (connection, window);

        xcb_flush (connection);
		int cenx;
		int ceny;
		

        xcb_generic_event_t *event;

		


		int pre_x = 0;
		int pre_y = 0;
		int new_x = 0;
		int new_y = 0;
		int deltax = 0;
		int deltay = 0;



        // char *mouse_mov = malloc(9);
        // char *mouse_but = malloc(6);
        // char *keyboard = malloc(6);
        xcb_xfixes_query_version(connection,4,0);
        xcb_xfixes_show_cursor(connection, screen->root);
        int flag = 0;

    pthread_t th[THREAD_NUM];
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);
    int i;
    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_create(&th[i], NULL, &startThread1, NULL) != 0) {
            perror("Failed to create the thread");
        }
    }

    srand(time(NULL));


    Task t = {
            .sockfd = sockfd,
    .connection = connection,
   .screen = screen,
   .window=window,
    .mask = mask, 
   .cenx = cenx,
   .ceny = ceny,
    .event = event,
   .pre_x = pre_x,
     .pre_y = pre_y,
    .new_x = new_x,
    .new_y = new_y,
    .deltax = deltax,
    .deltay = deltay,
    .flag = flag
        };
        submitTask(t);

    
    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
    
    return 0;
} 
