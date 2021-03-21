#include "common.h"

static const int bufsize = 1024;

static void write_remote(struct rdma_cm_id *id, uint32_t len)
{
  
  /*  Store the context and scatter/gather entry containing transfer
      information. */
  int ret;
  struct client_context *ctx = (struct client_context *)id->context;
  struct ibv_sge sge;

  /* Create an RDMA work request */
  struct ibv_send_wr wr, *bad_wr = NULL;
  memset(&wr, 0, sizeof(wr));

  /* Modify the parameters of the WR */
  wr.wr_id = (uintptr_t)id;
  wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
  wr.send_flags = IBV_SEND_SIGNALED;
  wr.imm_data = htonl(len);
  wr.wr.rdma.remote_addr = ctx->peer_addr;
  wr.wr.rdma.rkey = ctx->peer_rkey;

  /* Add the buffer location */
  if (len) {
    wr.sg_list = &sge;
    wr.num_sge = 1;
    sge.addr = (uintptr_t)ctx->buffer;
    sge.length = len;
    sge.lkey = ctx->buffer_mr->lkey;
  }

  /* Queue the RDMA send */
  ret = ibv_post_send(id->qp, &wr, &bad_wr);
  if (ret) {
    printf("Error %d while running ibv_post_send with qp %p", ret, id->qp);
  }

}

static void post_receive(struct rdma_cm_id *id)
{
  
  int ret;
  struct client_context *ctx = (struct client_context *)id->context;
  struct ibv_recv_wr wr, *bad_wr = NULL;
  struct ibv_sge sge;

  memset(&wr, 0, sizeof(wr));

  wr.wr_id = (uintptr_t)id;
  wr.sg_list = &sge;
  wr.num_sge = 1;

  sge.addr = (uintptr_t)ctx->msg;
  sge.length = sizeof(*ctx->msg);
  sge.lkey = ctx->msg_mr->lkey;

  ret = ibv_post_recv(id->qp, &wr, &bad_wr);
  if (ret) {
    printf("Error while posting receive request.\n");
  }
}

static void send_message(struct rdma_cm_id *id, char *message)
{
  struct client_context *ctx = (struct client_context *)id->context;

  /* Copy the data into the RDMA context buffer */
  size_t num_bytes = strlen(message);
  memcpy(ctx->buffer, message, num_bytes);

  /* Prepare for writing to the remote side */
  write_remote(id, num_bytes);
}


/* We first need to allocate a memory region and tell libibverbs that it's usable
by RDMA */
static void on_pre_conn(struct rdma_cm_id *id)
{
  struct client_context *ctx = (struct client_context *)id->context;

  /* Align the memory region allocation to a (typically) 4kB boundary */
  posix_memalign(
    (void **)&ctx->buffer, 
    sysconf(_SC_PAGESIZE), 
    bufsize
  );

  ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, bufsize, 0);
  if (!ctx->buffer_mr) {
    printf("Could not allocate MR.\n");
  }

  /* Align the memory region allocation to a (typically) 4kB boundary */
  posix_memalign(
    (void **)&ctx->msg,
    sysconf(_SC_PAGESIZE),
    sizeof(*ctx->msg)
  );

  ctx->msg_mr = ibv_reg_mr(rc_get_pd(), ctx->msg, sizeof(*ctx->msg), IBV_ACCESS_LOCAL_WRITE);
  if (!ctx->msg_mr) {
    printf("Could not allocate MR\n");
  };

  post_receive(id);
}

static void on_completion(struct ibv_wc *wc)
{
  struct rdma_cm_id *id = (struct rdma_cm_id *)(uintptr_t)(wc->wr_id);
  struct client_context *ctx = (struct client_context *)id->context;
  char *message = "Hello world, this is data sent over RDMA";

  /* Check if completion was a receive */
  if (wc->opcode & IBV_WC_RECV) {
    
    /* We get the MR information from the server */
    if (ctx->msg->id == MSG_MR)
    {
      ctx->peer_addr = ctx->msg->data.mr.addr;
      ctx->peer_rkey = ctx->msg->data.mr.rkey;
      printf("Received MR information.\n");
    } 
    else if (ctx->msg->id == MSG_READY) 
    {
      printf("Received RTS\n");
      send_message(id, message);
    }
    else if (ctx->msg->id == MSG_DONE) 
    {
      printf("Send completion acknowledgement\n");
      rdma_disconnect(id);
      return;
    }
    post_receive(id);
  }
}

static void rc_client_loop(const char *host, const char *port, void *context)
{
  
  int ret;
  struct addrinfo *addr;
  struct rdma_cm_id *conn = NULL;
  struct rdma_event_channel *ec = NULL;
  struct rdma_conn_param cm_params;

  ret = getaddrinfo(host, port, NULL, &addr);
  if (ret) {
    printf("Could not resolve input address");
  }

  ec = rdma_create_event_channel();
  if (!ec) {
    printf("Could not create RDMA event channel\n");
  }
  
  rdma_create_id(ec, &conn, NULL, RDMA_PS_TCP);
  if(rdma_resolve_addr(conn, NULL, addr->ai_addr, 2000)) {
    printf("Could not create RDMA ID\n");
  };

  freeaddrinfo(addr);

  conn->context = context;

  build_params(&cm_params);

  event_loop(ec, 1);

  rdma_destroy_event_channel(ec);
}

int main(int argc, char **argv)
{
  struct client_context ctx;

  if (argc != 3) {
    fprintf(stderr, "Invalid number of input arguments\n    Usage: %s <server_ip> <port>\n", argv[0]);
    return 1;
  }

  rc_init(
    on_pre_conn,
    NULL,
    on_completion,
    NULL);

  rc_client_loop(argv[1], argv[2], &ctx);

  return 0;
}

