// Basic stuff
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// Networking + RDMA stuff
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <rdma/rsocket.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <infiniband/verbs.h>

#include "../common/rdma_common.h"

#define MAX 10000000 
#define PORT 6969  
#define SA struct sockaddr 
#define WIDTH 1920
#define HEIGHT 1080

pthread_mutex_t lock;

int main(int argc, char *argv[]) 
{   
        
    int ret;
    
    // Use rdma_cm to create an RDMA RC connection.
    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;

    hints.ai_flags = RAI_PASSIVE;       // According to rdma_cm docs, allows both sides to transmit data
    hints.ai_port_space = RDMA_PS_TCP;  // Allocate a TCP port for the connection setup
    ret = rdma_getaddrinfo(argv[1], argv[2], &hints, &host_res);
    if (ret)
    {
        fprintf(stderr, "Could not resolve host.\n");
        return 1;
    }

    new(struct ibv_qp_init_attr, init_attr);
    struct rdma_cm_id *connid;
    
    // Allow the same QP to be used by two threads with 256 bytes
    // allocated for inline data, such as HID input. Also create
    // it as reliable connected (TCP equivalent)
    init_attr.cap.max_send_wr = 2;
    init_attr.cap.max_recv_wr = 2;
    init_attr.cap.max_send_sge = 2;
    init_attr.cap.max_recv_sge = 2;
    init_attr.cap.max_inline_data = 256;
    init_attr.qp_type = IBV_QPT_RC;

    ret = rdma_create_ep(&connid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection.\n");
    }
    if (!(init_attr.cap.max_inline_data >= 256 && init_attr.cap.max_send_sge >= 2
        && init_attr.qp_type == IBV_QPT_RC))
    {
        fprintf(stderr, 
                "Connection does not support features required by Telescope:\n"
                "Max inline data: %d\t(req. %d)\n"
                "QP Type:         %d\t(req. %d - IBV_QPT_RC)\n"
                "Max send SGEs:   %d\t(req. %d)\n",
                init_attr.cap.max_inline_data, 256, 
                init_attr.qp_type, IBV_QPT_RC,
                init_attr.cap.max_send_sge, 2
        );
        rdma_destroy_ep(connid);
        return 1;
    }

    void *hello = calloc(256, 1);
    int send_flags;
    struct ibv_mr *send_mr = rdma_reg_msgs(connid, hello, 256);
    struct ibv_wc wc;

    while(1)
    {
        strcpy(hello, "Hello server!");
        printf("Sending %s\n", (char *) hello);
        rdma_post_send(connid, NULL, hello, 256, send_mr, IBV_SEND_INLINE);
        while ((ret = rdma_get_send_comp(connid, &wc)) == 0);
        if (ret < 0) {
            fprintf(stderr, "Error sending data\n");
        }
        usleep(1000000);
    }

} 
