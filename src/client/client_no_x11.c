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
#define INLINE_BUFSIZE 24
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
    if (!(init_attr.cap.max_inline_data >= INLINE_BUFSIZE
        && init_attr.qp_type == IBV_QPT_RC))
    {
        fprintf(stderr, 
                "Connection does not support features required by Telescope:\n"
                "Max inline data: %d\t(req. %d)\n"
                "QP Type:         %d\t(req. %d - IBV_QPT_RC)\n",
                init_attr.cap.max_inline_data, INLINE_BUFSIZE, 
                init_attr.qp_type, IBV_QPT_RC
        );
        rdma_destroy_ep(connid);
        return 1;
    }

    void *hello = calloc(INLINE_BUFSIZE, 1);
    int send_flags;
    struct ibv_mr *send_mr = rdma_reg_msgs(connid, hello, INLINE_BUFSIZE);
    struct ibv_wc wc;

    ret = rdma_connect(connid, NULL);
    *(int *) hello = T_MSG_KEEP_ALIVE;

    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
    int speed_test;
    int sdur = 10000;
    int n = 11;
    int cn = 0;

    printf("\"Limit\",\"Speed\"\n");

    while(1)
    {
        usleep(sdur);
        rdma_post_send(connid, NULL, hello, INLINE_BUFSIZE, send_mr, IBV_SEND_INLINE);
        while ((ret = rdma_get_send_comp(connid, &wc)) == 0);
        if (ret < 0) {
            fprintf(stderr, "Error sending data\n");
        }
        if (!ret)
        {
            continue;
        }
        speed_test++;
        gettimeofday(&t1, NULL);
        if (t1.tv_sec - t0.tv_sec >= 1)
        {
            printf("%d,%d\n", speed_test, sdur);
            speed_test = 0;
            if (cn++ > n)
            {
                if (sdur <= 100)
                {
                    sdur -= 1;
                }
                else if (sdur <= 1000)         
                {
                    sdur -= 10;
                }                
                else if (sdur <= 5000)
                {
                    sdur -= 100;
                }
                else
                {
                    sdur -= 200;
                }
                cn = 0;
            }
            if (sdur < 0)
            {
                exit(0);
            }
            gettimeofday(&t0, NULL);
        }
    }
} 
