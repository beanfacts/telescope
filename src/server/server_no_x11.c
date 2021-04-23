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

typedef struct
{
    struct rdma_cm_id   *connid;        // RDMA connection identifier
    Display             *dsp;           // X11 display
    XImage              *ximage;
    void                *rcv_buf;
    int                 rcv_bufsize;
    int                 poll_usec;
} ControlArgs;


void *control(void *void_args)
{
    ControlArgs *args = (ControlArgs *) void_args;
    struct ibv_wc *rcv_wc;
    struct ibv_mr *rcv_mr;
    int ret;
     
    // Register receive buffer memory region for signalling data
    rcv_mr = rdma_reg_msgs(args->connid, args->rcv_buf, args->rcv_bufsize);
    if (!rcv_mr)
    {
        fprintf(stderr, "Could not register memory region.\n");
    }

    // Wait for client to send a request
    ret = rdma_post_recv(args->connid, NULL, args->rcv_buf, args->rcv_bufsize, rcv_mr);
    if (ret)
    {
        fprintf(stderr, "Could not post the request to the send queue.\n");
    }

    // Poll for RDMA receive completion
    while ((ret = rdma_get_recv_comp(args->connid, rcv_wc)) == 0)
    {
        usleep(args->poll_usec);
    }
    if (ret < 0)
    {
        fprintf(stderr, "Error receiving data from the client.\n");
    }
    printf("Received data!!!!!!!!!!!!\n");
    printf("Server said: %s\n", (char *) args->rcv_buf);
    memset(args->rcv_buf, 0, 256);
}

int main(int argc, char* argv[])
{

    int ret;

    // Use rdma_cm to create an RDMA RC connection.
    // Resolve the address into a connection identifier
    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;

    hints.ai_port_space = RDMA_PS_TCP;
    ret = rdma_getaddrinfo(argv[1], argv[2], &hints, &host_res);
    if (ret)
    {
        fprintf(stderr, "Could not resolve host.\n");
        return 1;
    }

    new(struct ibv_qp_init_attr, init_attr);
    struct rdma_cm_id *sockid, *connid;
    
    // Allow the same QP to be used by two threads with 256 bytes
    // allocated for inline data, such as HID input. Also create
    // it as reliable connected (TCP equivalent)
    init_attr.cap.max_send_wr = 2;
    init_attr.cap.max_recv_wr = 2;
    init_attr.cap.max_send_sge = 2;
    init_attr.cap.max_recv_sge = 2;
    init_attr.cap.max_inline_data = 256;
    init_attr.qp_type = IBV_QPT_RC;
    ret = rdma_create_ep(&sockid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection\n");
        return 1;
    }

    // Start listening for incoming connections
    ret = rdma_listen(sockid, 5);
    if (ret)
    {
        fprintf(stderr, "Failed to listen on RDMA channel.\n");
    }

    while (1)
    {
        ret = rdma_get_request(sockid, &connid);
        if (ret)
        {
            fprintf(stderr, "Failed to read connection request.\n");
            usleep(10000);
            continue;
        }

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
        
        // Disconnect client if there isn't enough inline buffer space or SGEs.
        if (!(conn_attr.cap.max_inline_data >= 256 && conn_attr.cap.max_send_sge >= 2
            && conn_attr.qp_type == IBV_QPT_RC))
        {
            fprintf(stderr, 
                "Connection does not support features required by Telescope:\n"
                "Max inline data: %d\t(req. %d)\n"
                "QP Type:         %d\t(req. %d - IBV_QPT_RC)\n"
                "Max send SGEs:   %d\t(req. %d)\n",
                conn_attr.cap.max_inline_data, 256, 
                conn_attr.qp_type, IBV_QPT_RC,
                conn_attr.cap.max_send_sge, 2
            );
            rdma_destroy_ep(connid);
            continue;
        }

        // Accept the connection if the checks pass
        ret = rdma_accept(connid, NULL);
        if (ret)
        {
            fprintf(stderr, "Could not accept connection");
            continue;
        }

        pthread_t id1;

        ControlArgs *args_control = malloc(sizeof(ControlArgs));
        void *recieve_buffer = malloc(256);

        args_control->connid = connid;
        args_control->dsp = NULL;
        args_control->ximage = NULL;
        args_control->rcv_buf = recieve_buffer;
        args_control->rcv_bufsize = 256;
        args_control->poll_usec = 1000;

        // Create a new thread to accept the client connection.
        pthread_create(&id1, NULL, control, (void *) args_control);


    }
}
