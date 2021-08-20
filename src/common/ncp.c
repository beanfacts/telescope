/*
    Telescope Negotiation Channel Protocol
    Copyright (C) 2021 Tim Dettmar
    SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "rdma_common.h"
#include <rdma/rdma_cma.h>

/* Telescope NCP Library Struct */
typedef struct {
    
    /* Metadata channel buffers */
    struct ibv_mr *snd_mr;
    struct ibv_mr *rcv_mr;

} T_NCData;

/* Initialize client buffers and connect */
int init_client();
    
/* Initialize server buffers and listen */
int init_server(struct sockaddr_in *clientid, struct rdma_cm_id *connid, int bufsize) {
    
    int ret;
    void *receive_buffer, *send_buffer;
    ret = posix_memalign(&receive_buffer, sysconf(_SC_PAGE_SIZE), bufsize);
    if (ret || !receive_buffer) { 
        goto buf_error;
    }
    ret = posix_memalign(&send_buffer, sysconf(_SC_PAGE_SIZE), bufsize);
    if (ret || !send_buffer) {
        goto buf_error;
    }

    buf_error:
        if (receive_buffer) {
            free(receive_buffer);
        }
        if (send_buffer) {
            free(send_buffer);
        }
        return -1;
}