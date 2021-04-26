/* Telescope Server */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/extensions/XShm.h>
#include <X11/extensions/XTest.h>
#include <sys/socket.h> 

#include <sys/time.h>

#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <infiniband/verbs.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h> 

#include "../common/rdma_common.h"

#include <fcntl.h>

#define __T_DEBUG__
#define INLINE_BUFSIZE 32
#define TOTAL_WR 1
#define TOTAL_SGE 1
static struct rdma_cm_id *sockid, *connid;

typedef struct
{
    struct rdma_cm_id   *connid;        // RDMA connection identifier
    Display             *dsp;           // X11 display
    XImage              *ximage;
    void                *rcv_buf;
    int                 poll_usec;
} ControlArgs;

/*  Telescope's main control thread.
    Each thread is responsible for one client; the connection handles sending
    and receiving of data over RDMA, as well as control of the mouse if it is
    enabled.
*/
void *control(void *void_args)
{
    ControlArgs *args = (ControlArgs *) void_args;
    struct ibv_wc rcv_wc;
    struct ibv_mr *rcv_mr;
    int ret;

    // The caller of this function can specify whether to use a specific
    // receive side buffer, otherwise the thread will allocate its own
    // memory to store received messages. To force the thread to allocate
    // its own memory, set the argument value to NULL.
    if(!(args->rcv_buf))
    {
        args->rcv_buf = calloc(INLINE_BUFSIZE, 1);
    }
     
    // Register receive buffer memory region for signalling data
    rcv_mr = rdma_reg_msgs(args->connid, args->rcv_buf, INLINE_BUFSIZE);
    if (!rcv_mr)
    {
        fprintf(stderr, "Could not register memory region.\n");
    }

    printf("Registered buffer\n");

    // Inform the hardware to place data received into this buffer
    ret = rdma_post_recv(args->connid, NULL, args->rcv_buf, INLINE_BUFSIZE, rcv_mr);
    if (ret)
    {
        fprintf(stderr, "Could not post the request to the send queue.\n");
    }
    printf("Posted receive\n");

    struct ibv_cq *notified_cq;
    void *notified_cq_context;

    while (1)
    {

        // Request the hardware notify the client when the transmission is
        // completed.
        ibv_req_notify_cq(args->connid->recv_cq, 0);
        if (ret)
        {
            fprintf(stderr, "Error: could not request a completion event\n");
        }

        // Waits until a completion event has been generated by the hardware
        ret = ibv_get_cq_event(args->connid->recv_cq_channel, &notified_cq, &notified_cq_context);
        if (ret)
        {
            fprintf(stderr, "Could not wait for a completion event.\n");
        }

        printf("Received inline data.\n");
        printf("Server said: %s\n", (char *) args->rcv_buf);

    }
 
}

// static struct rdma_cm_id *sockid, *connid;

int main(int argc, char* argv[])
{

    int ret;

    // Use rdma_cm to create an RDMA RC connection.
    // Resolve the address into a connection identifier
    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;

    printf("Attempting to listen to %s:%s\n", argv[1], argv[2]);

    hints.ai_port_space = RDMA_PS_TCP;
    hints.ai_flags = RAI_PASSIVE;
    
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
    // init_attr.cap.max_recv_wr = TOTAL_WR;
    // init_attr.cap.max_recv_sge = TOTAL_SGE;
    // init_attr.cap.max_inline_data = INLINE_BUFSIZE;
    // init_attr.qp_type = IBV_QPT_RC;

    init_attr.cap.max_send_wr = init_attr.cap.max_recv_wr = 1;
	init_attr.cap.max_send_sge = init_attr.cap.max_recv_sge = 1;
	init_attr.cap.max_inline_data = 16;
	init_attr.sq_sig_all = 1;
    ret = rdma_create_ep(&sockid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection\n");
        return 1;
    }
    printf("Created RDMA connection\n");

    // Start listening for incoming connections
    ret = rdma_listen(sockid, 0);
    if (ret)
    {
        fprintf(stderr, "Failed to listen on RDMA channel.\n");
    }
    printf("Listening on RDMA channel\n");
    printf("%d",sockid);
    printf( "%d",&connid);
    //ret = rdma_get_request(sockid, &connid);

    while (1)
    {
        ret = rdma_get_request(sockid, &connid);
        if (ret)
        {
            fprintf(stderr, "Failed to read connection request.\n");
            usleep(10000);
            continue;
        }
        printf("Got a connection request.\n");

        // Determine whether the connection supports the features required
        // for Telescope
        new(struct ibv_qp_init_attr, conn_attr);
        new(struct ibv_qp_attr, conn_qp_attr);
        
        // Disconnect client if QP information cannot be retrieved.
        ret = ibv_query_qp(connid->qp, &conn_qp_attr, IBV_QP_CAP, &conn_attr);
        if (ret)
        {
            fprintf(stderr, "Could not get QP information for client connection.\n");
            rdma_destroy_ep(connid);
            continue;
        }
        printf("Received QP information\n");
        printf("Preparing to accept client\n");

        // Accept the connection if the checks pass
        ret = rdma_accept(connid, NULL);
        if (ret)
        {
            fprintf(stderr, "Could not accept connection");
            continue;
        }

        printf("Accepted client. Starting thread\n");

        struct ibv_wc rcv_wc;
        int image_bytes = 1000;
        
        struct ibv_mr *image_mr;
        struct ibv_mr *inline_mr;

        void *buffer = calloc(image_bytes, 1);
        memset(buffer, 1, image_bytes);
        void *inbuf = calloc(INLINE_BUFSIZE, 1);
        
        image_mr = ibv_reg_mr(connid->pd, (void *) buffer, image_bytes, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
        if (!image_mr)
        {
            fprintf(stderr, "Error registering memory region: %s", strerror(errno));
        }

        inline_mr = rdma_reg_msgs(connid, inbuf, INLINE_BUFSIZE);
        if (!inline_mr)
        {
            fprintf(stderr, "Error registering inline memory region: %s", strerror(errno));
        }

        ret = rdma_post_recv(connid, NULL, inbuf, INLINE_BUFSIZE, inline_mr);
        if (ret)
        {
            fprintf(stderr, "Could not post the receive request to the queue: %s\n", strerror(errno));
        }

        while ((ret = rdma_get_recv_comp(connid, &rcv_wc)) == 0)
        {
            printf("Waiting for incoming data...\n");
        }

        
        T_InlineBuff *mem_msg = (T_InlineBuff *) inbuf;
        
        printf( "------- Client information -------\n"
        "(Type: %20lu) (Addr: %20lu)\n"
        "(Size: %20lu) (Rkey: %20lu)\n"
        "(Bufs: %20lu)\n" 
        "----------------------------------\n",
        (uint64_t) mem_msg->data_type, (uint64_t) mem_msg->addr, (uint64_t) mem_msg->size,
        (uint64_t) mem_msg->rkey, (uint64_t) mem_msg->numbufs);

        for(int i=0; i < sizeof(T_InlineBuff) ; i++)
        {
            printf("%hhx", *((char *) buffer + i));
        }
        printf("\n");

        ret = rdma_post_write(connid, NULL, buffer, image_bytes, image_mr, 0, (uint64_t) mem_msg->addr, mem_msg->rkey);
        if (ret)
        {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            continue;
        }
        while ((ret = rdma_get_send_comp(connid, &rcv_wc)) == 0)
        {
            printf("what\n");
        }
        if (rcv_wc.status)
        {
            fprintf(stderr, "Error RDMA: %d\n", rcv_wc.status);
            continue;
        }
        printf("Transfer complete!!!\n");

    }
}
