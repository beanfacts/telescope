#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define SAI struct sockaddr_in
#define THDR 0x74646d61

/*  Create a socket with provided details.
    Parameters:
        host:       Host information
                    Must be of type in_addr_t. IPv4 only.
        port:       Port number in CPU byte order
        socktype:   Socket type; TCP = SOCK_STREAM; UDP = SOCK_DGRAM; SCTP = SOCK_SEQPACKET
        config:     Pointer to a struct sockaddr_in populated when the connection is successfully established
    Returns:
        int:    File descriptor of the created socket
                On error, -1 is returned.
*/ 
int td_create_server(in_addr_t host, uint16_t port, int socktype, SAI *config);

/*  Connect to a server with provided details.
    Parameters:
        host:       Host information
                    Must be of type in_addr_t. IPv4 only.
        port:       Port number in CPU byte order.
        socktype:   Socket type; TCP = SOCK_STREAM; UDP = SOCK_DGRAM; SCTP = SOCK_SEQPACKET
        config:     Pointer to a struct sockaddr_in populated when the connection is successfully established
    Returns:
        int:    File descriptor of the created socket
                On error, -1 is returned.
*/
int td_create_client(in_addr_t host, uint16_t port, int socktype, SAI *config);