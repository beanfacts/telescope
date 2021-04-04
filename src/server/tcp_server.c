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


void func(int sockfd) 
{ 
    
    // Open the X11 display
    Display *Xdisp;
    Window Xrwin;

    Xdisp = XOpenDisplay(0);
    Xrwin = XRootWindow(Xdisp, 0);
    
    // Allocate buffer size for reading
	char *buff = calloc(MAX, sizeof(char));
    
    int n; 
    int size_b;

    for (;;) { 

        memset(buff, 0, MAX); 
  
        read(sockfd, buff, 16); 
        if(buff[0] == 1 | buff[0] == 2) {
            size_b = 6;
        }
        else if (buff[0] == 0)
        {
            size_b = 9;
        }

        // Decode packet and perform movement action
        ControlPacket pkt = decode_packet(buff, size_b);
        process_input(pkt, Xdisp, Xrwin);
        
        memset(buff, 0,MAX); 
        n = 0; 
   
        write(sockfd, buff, sizeof(buff)); 

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
  
    // Create socket and set up addresses

    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    memset(&servaddr, 0, sizeof(servaddr)); 
  
    int PORT = (short) atoi(argv[1]);

    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Bind address

    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
    
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  

    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n"); 
  
    func(connfd); 
    close(sockfd); 
    return 0;
} 
