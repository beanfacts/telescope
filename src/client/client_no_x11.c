// Basic stuff
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

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
#define INLINE_BUFSIZE 32
#define TOTAL_WR 1
#define TOTAL_SGE 1

static struct rdma_cm_id *connid;

int main(int argc, char *argv[]) 
{    
    int ret;
    
    // Use rdma_cm to create an RDMA RC connection.
    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;
    
    hints.ai_port_space = RDMA_PS_TCP;  // Allocate a TCP port for the connection setup
    
    printf("Attempting to connect to %s:%s\n", argv[1], argv[2]);
    ret = rdma_getaddrinfo(argv[1], argv[2], &hints, &host_res);
    if (ret)
    {
        fprintf(stderr, "Could not resolve host.\n");
        return 1;
    }

    new(struct ibv_qp_init_attr, init_attr);
    
    // Allow the same QP to be used by two threads with 256 bytes
    // allocated for inline data, such as HID input. Also create
    // it as reliable connected (TCP equivalent)
    init_attr.cap.max_send_wr = TOTAL_WR;
    init_attr.cap.max_recv_wr = TOTAL_WR;
    init_attr.cap.max_send_sge = TOTAL_SGE;
    init_attr.cap.max_recv_sge = TOTAL_SGE;
    init_attr.cap.max_inline_data = INLINE_BUFSIZE;
    init_attr.qp_type = IBV_QPT_RC;
    init_attr.qp_context = connid;
    init_attr.sq_sig_all = 1;

    ret = rdma_create_ep(&connid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection.\n");
    }

    void *hello = calloc(INLINE_BUFSIZE, 1);
    int send_flags;
    struct ibv_mr *send_mr = rdma_reg_msgs(connid, hello, INLINE_BUFSIZE);

    ret = rdma_connect(connid, NULL);
    *(int *) hello = T_MSG_KEEP_ALIVE;

    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
    int speed_test;
    int sdur = 10000;
    int n = 11;
    int cn = 0;

    printf("\"Limit\",\"Speed\"\n");

    struct ibv_mr *image_mr;
    struct ibv_wc wc;

    int image_bytes = 1000;
    void *buffer = calloc(image_bytes, 1);
    
    image_mr = ibv_reg_mr(connid->pd, (void *) buffer, image_bytes, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!image_mr)
    {
        fprintf(stderr, "Error registering memory region: %s", strerror(errno));
    }

    T_InlineBuff *mem_msg = (T_InlineBuff *) hello;
    mem_msg->data_type = T_INLINE_DATA_CLI_MR;
    mem_msg->addr = image_mr->addr;
    mem_msg->size = image_bytes;
    mem_msg->rkey = image_mr->rkey;
    mem_msg->numbufs = 1;

    for(int i=0; i < sizeof(T_InlineBuff) ; i++)
    {
        printf("%hhx", *((char *) hello + i));
    }
    printf("\n");

    printf("Before:\n");
    for(int i = 0; i < 1000; i++)
    {
        printf("%hhx", *((char *) buffer + i));
    }
    printf("\n");
    
    printf( "------- Client information -------\n"
        "(Type: %20lu) (Addr: %20lu)\n"
        "(Size: %20lu) (Rkey: %20lu)\n"
        "(Bufs: %20lu)\n" 
        "----------------------------------\n",
        (uint64_t) mem_msg->data_type, (uint64_t) mem_msg->addr, (uint64_t) mem_msg->size,
        (uint64_t) mem_msg->rkey, (uint64_t) mem_msg->numbufs);

    rdma_post_send(connid, NULL, hello, INLINE_BUFSIZE, send_mr, 0);
    while ((ret = rdma_get_send_comp(connid, &wc)) == 0)
    {
        printf("waiting\n");
    }
    if (ret < 0)
    {
        fprintf(stderr, "Error sending data\n");
    }
    printf("rdma ready\n");
    sleep(5);

    printf("After:\n");
    for(int i = 0; i < 1000; i++)
    {
        printf("%hhx", *((char *) buffer + i));
    }
    printf("\n");

} 
