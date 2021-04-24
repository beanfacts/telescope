#include "rdma_common.h"
#include <sys/time.h>

// no longer needed - using pure RDMACM

/*
int full_xfer(int mode, int connfd, void *data, int len, int min_xfer_threshold, float timeout, int usleep_duration)
{
    ssize_t total_bytes = 0;
    ssize_t xfer_bytes = 0;
    int attempts;
    clock_t tick, tock = 0;

    while (total_bytes < len)
    {
        if (mode == 0)
        {
            xfer_bytes = rread(connfd, (void *) data, (len - total_bytes));
        }
        else if (mode == 1)
        {
            xfer_bytes = rwrite(connfd, (void *) data, (len - total_bytes));
        }
        total_bytes += xfer_bytes;
        if (xfer_bytes <= min_xfer_threshold)
        {
            if (tick)
            {
                tock = clock();
                if ((tock - tick) / (float) CLOCKS_PER_SEC > timeout)
                {
                    fprintf(stderr, "RDMA transfer request timed out.\n");
                    return 0;
                }
            }
            else
            {
                tick = clock();
            }
            usleep(usleep_duration);
        }
        else
        {
            tick = 0;
            attempts = 0;
        }
    }
}

T_List *get_displays(int connfd)
{
    int ret;
    T_MsgHeader hdr = {
        .length = 0,
        .msg_type = T_REQ_SCREEN_DATA,
        .num_msgs = 0
    };
    
    ret = full_xfer(XFER_WRITE, connfd, (void *) &hdr, sizeof(T_MsgHeader), 0, msec(3000), msec(10));
    if (!ret)
        goto data_error;

    memset(&hdr, 0, sizeof(T_MsgHeader));
    
    full_xfer(XFER_READ, connfd, (void *) &hdr, sizeof(T_MsgHeader), 0, msec(3000), msec(10));
    if (!ret)
        goto data_error;

    if (hdr.msg_type == T_META_SCREEN_DATA && hdr.num_msgs && hdr.num_msgs <= 16)
    {
        T_ScreenData *screen_data = malloc(sizeof(T_ScreenData) * hdr.num_msgs);
        ret = full_xfer(XFER_READ, connfd, (void *) screen_data, sizeof(T_ScreenData) * hdr.num_msgs, 0, msec(3000), msec(10));
        if (!ret)
            goto data_error;
        else
        {
            T_List *list = malloc(sizeof(T_List));
            list->elements = hdr.num_msgs;
            list->data = screen_data;
        }
    }

    data_error:
        fprintf(stderr, "Could not request display metadata.\n");
        return NULL;

}

int req_image(int connfd, int dpy_index, off_t offset, int buf_index, int chroma)
{
    int ret;

    T_MsgHeader hdr = {
        .length = 0,
        .msg_type = T_REQ_BUFFER_WRITE,
        .num_msgs = 1
    };

    T_WriteReq wrq = {
        .buf_index = buf_index,
        .dpy_index = dpy_index,
        .offset = offset
    };
    
    ret = full_xfer(XFER_WRITE, connfd, (void *) &hdr, sizeof(T_MsgHeader), 0, msec(1000), msec(10));
    if (!ret)
    {
        goto data_error;
    }

    ret = full_xfer(XFER_WRITE, connfd, (void *) &wrq, sizeof(T_WriteReq), 0, msec(1000), msec(10));
    if (!ret)
    {
        goto data_error;
    }

    data_error:
        fprintf(stderr, "Could not request image from display %d\n", dpy_index);
        return NULL;
}

int prepare_image(int connfd, int dpy_index, int chroma)
{
    int ret;
    T_MsgHeader hdr = {
        .length = 0,
        .msg_type = T_REQ_SERVER_PREPARE,
        .num_msgs = 1
    };

    T_PrepareReq ppr = {
        .chroma = chroma,
        .dpy_index = dpy_index
    };

    ret = full_xfer(XFER_WRITE, connfd, (void *) &hdr, sizeof(T_MsgHeader), 0, msec(1000), msec(10));
    if (!ret)
    {
        goto data_error;
    }

    ret = full_xfer(XFER_WRITE, connfd, (void *) &ppr, sizeof(T_PrepareReq), 0, msec(1000), msec(10));
    if (!ret)
    {
        goto data_error;
    }

    data_error:
        fprintf(stderr, "Could not request image from display %d\n", dpy_index);
        return NULL;

}
*/