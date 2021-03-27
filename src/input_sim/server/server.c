#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <stdbool.h>
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#define MAX 90
#define SA struct sockaddr

typedef struct {
    int controlPktType;     // 0 -> Mouse Move, 1 -> Mouse Click, 2 -> Keyboard
    
    int mmove_dx;           // Mouse movement x (px)
    int mmove_dy;           // Mouse movement y (px)
    
    int press_keycode;      // Keycode
    int press_state;        // State; 0 -> released, 1 -> pressed

} ControlPacket;

ControlPacket decode_packet(char *stream, int size) {
    
    ControlPacket pkt;

    pkt.controlPktType = -1;
    
    if (size == 9 || stream[0] == 0) 
    {
        // Detect mouse movement
        pkt.controlPktType = (int) stream[0];
        pkt.mmove_dx = *(int *) (&stream[1]);
        pkt.mmove_dy = *(int *) (&stream[5]);
	
	printf("decode -> ");

	for (int x = 0; x < 9; x++) {
		printf("(%d) ", (unsigned char) stream[x]);
	}

	printf("\n");

	printf("decode move: %d, %d\n", pkt.mmove_dx, pkt.mmove_dy);
	if (pkt.mmove_dx > 1000 || pkt.mmove_dx < -1000) { pkt.mmove_dx = 0; }
        if (pkt.mmove_dy > 1000 || pkt.mmove_dy < -1000) { pkt.mmove_dy = 0; }
	
    } 
    else if (size == 6 && (stream[0] == 1 || stream[0] == 2)) 
    {
        // Detect keyboard or mouse button press
        pkt.controlPktType = (int) stream[0];
        pkt.press_keycode = *(int *) (&stream[1]);
        pkt.press_state = (int) stream[5];
    }

    return pkt;
}

bool process_input(ControlPacket pkt, Display *disp, Window xwin) {
    
    bool r = false;

    switch (pkt.controlPktType) {
        case 0:
            XWarpPointer(disp, xwin, 0, 0, 0, 0, 0, pkt.mmove_dx, pkt.mmove_dy);
            printf("move mouse: %d, %d\n", pkt.mmove_dx, pkt.mmove_dy);
            r = true;
            break;
        case 1:
            XTestFakeButtonEvent(disp, pkt.press_keycode, pkt.press_state, 0);
            printf("press mouse: %d, %d\n", pkt.press_keycode, pkt.press_state);
            r = true;
            break;
        case 2:
            XTestFakeKeyEvent(disp, pkt.press_keycode, pkt.press_state, 0);
            printf("press keyboard: %d, %d\n", pkt.press_keycode, pkt.press_state);
            r = true;
            break;
    }

    if (r) {
        XFlush(disp);
    }
    return r;

}

// Function designed for chat between client and server. 
void func(int sockfd) 
{ 
    
    /* Open X11 display */
    Display *Xdisp;
    Window Xrwin;

    Xdisp = XOpenDisplay(0);
    Xrwin = XRootWindow(Xdisp, 0);
    
    //char buff[MAX]; 
	char *buff = calloc(MAX, sizeof(char));
    
    int n; 
    int size_b;
    //printf("%ld \n", sizeof(buff));
    // infinite loop for chat 
    for (;;) { 
        memset(buff,0 ,MAX); 
  
        // read the message from client and copy it in buffer 
        read(sockfd, buff, 16); 
        if(buff[0] == 1 | buff[0] == 2) {
            size_b = 6;
        }
        else if (buff[0] == 0)
        {
            size_b = 9;
        }

        ControlPacket pkt = decode_packet(buff, size_b);
        process_input(pkt, Xdisp, Xrwin);
        
        // print buffer which contains the client contents 
        //for(int i =0; i < size, ) 
        //int *ptr = (int * )(buff+1);
        //printf("From client: %d \t To client : \n", *(buff+0) ); 
        
        printf("tim ");

        for(int i=0; i < size_b; i++){
            printf("%hhx", *(buff+i));
        }
        
        printf("\n");
        
        memset(buff, 0,MAX); 
        n = 0; 
        // copy server message in the buffer 
        // while ((buff[n++] = getchar()) != '\n') 
        //     ; 
  
        // and send that buffer to client
        //*buff = "hello world"; 
        write(sockfd, buff, sizeof(buff)); 
  
        // if msg contains "Exit" then server exit and chat ended. 
        // if (strncmp("exit", buff, 4) == 0) { 
        //     printf("Server Exit...\n"); 
        //     break; 
        // } 
    } 
} 
  
// Driver function 
int main(int argc, char **argv) 
{ 
    
    if (argc < 2) {
        printf("No port provided!\n");
        exit(1);
    }
    
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    memset(&servaddr, 0, sizeof(servaddr)); 
  
    int PORT = (short) atoi(argv[1]);

    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n"); 
  
    // Function for chatting between client and server 
    func(connfd); 
  
    // After chatting close the socket 
    close(sockfd); 
    return 0;
} 
