#include "common.h"

static const int bufsize = 1024;

static void send_message(struct rdma_cm_id *id)
{

  struct server_context *ctx = (struct server_context *)id->context;
  struct ibv_send_wr wr, *bad_wr = NULL;
  struct ibv_sge sge;
  int ret;

  memset(&wr, 0, sizeof(wr));

  wr.wr_id = (uintptr_t)id;
  wr.opcode = IBV_WR_SEND;
  wr.sg_list = &sge;
  wr.num_sge = 1;
  wr.send_flags = IBV_SEND_SIGNALED;

  sge.addr = (uintptr_t)ctx->msg;
  sge.length = sizeof(*ctx->msg);
  sge.lkey = ctx->msg_mr->lkey;

  ret = ibv_post_send(id->qp, &wr, &bad_wr);
  if (ret) {
    printf("Could not post the send request.\n");
  }
}

static void post_receive(struct rdma_cm_id *id)
{
  struct ibv_recv_wr wr, *bad_wr = NULL;
  int ret;

  memset(&wr, 0, sizeof(wr));

  wr.wr_id = (uintptr_t)id;
  wr.sg_list = NULL;
  wr.num_sge = 0;

  ret = ibv_post_recv(id->qp, &wr, &bad_wr);
  if (ret) {
    printf("Could not post the work request to the receive queue.\n");
  }
  
}

static void on_pre_conn(struct rdma_cm_id *id)
{
  struct server_context *ctx = (struct server_context *)malloc(sizeof(struct server_context));

  id->context = ctx;

  posix_memalign(
    (void **)&ctx->buffer,
    sysconf(_SC_PAGESIZE),
    bufsize
  );

  ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, bufsize,
    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE
  );

  if (!ctx->buffer_mr) {
    printf("Could not allocate MR\n");
  }

  posix_memalign(
    (void **)&ctx->msg,
    sysconf(_SC_PAGESIZE),
    sizeof(*ctx->msg)
  );
  
  ctx->msg_mr = ibv_reg_mr(rc_get_pd(), ctx->msg, sizeof(*ctx->msg), 0);
  if (!ctx->msg_mr) {
    printf("Could not register MR.\n");
  }

  post_receive(id);
}

static void on_connection(struct rdma_cm_id *id)
{
  struct server_context *ctx = (struct server_context *)id->context;

  ctx->msg->id = MSG_MR;
  ctx->msg->data.mr.addr = (uintptr_t)ctx->buffer_mr->addr;
  ctx->msg->data.mr.rkey = ctx->buffer_mr->rkey;

  send_message(id);
}

static void on_completion(struct ibv_wc *wc)
{
  
  struct rdma_cm_id *id = (struct rdma_cm_id *) (uintptr_t) wc->wr_id;
  struct server_context *ctx = (struct server_context *) id->context;

  /*  According to Mellanox docs:
      Set value of IBV_WC_RECV so consumers can test if a completion is a
      receive by testing (opcode & IBV_WC_RECV).
  */
  if (wc->opcode == IBV_WC_RECV_RDMA_WITH_IMM) {

    uint32_t size = ntohl(wc->imm_data);

    /* Data transfer complete */
    if (size == 0) {
      ctx->msg->id = MSG_DONE;
      send_message(id);

    /* Sending data */
    } else {

      /* Receieved data */
      printf("%i bytes received.\n", size);
      printf("Incoming data --> %s", ctx->buffer);
      post_receive(id);

      /* Send ACK to user */
      ctx->msg->id = MSG_READY;
      send_message(id);

    }
  }
}

static void on_disconnect(struct rdma_cm_id *id)
{
  struct server_context *ctx = (struct server_context *)id->context;

  ibv_dereg_mr(ctx->buffer_mr);
  ibv_dereg_mr(ctx->msg_mr);

  free(ctx->buffer);
  free(ctx->msg);

  printf("Transfer complete\n");

  free(ctx);
}

void rc_server_loop(const char *port)
{
  struct sockaddr_in6 addr;
  struct rdma_cm_id *listener = NULL;
  struct rdma_event_channel *ec = NULL;
  int ret;

  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET;
  addr.sin6_port = htons(atoi(port));

  ec = rdma_create_event_channel();
  
  ret = rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP);
  if (ret) {
    printf("Error allocating communication identifier.\n");
  }

  ret = rdma_bind_addr(listener, (struct sockaddr *)&addr);
  if (ret) {
    printf("Error binding RDMA identifer to source address.\n");
  }

  ret = rdma_listen(listener, 5);
  if (ret) {
    printf("Error occurred while trying to listen for connections.\n");
  }

  event_loop(ec, 0);

  rdma_destroy_id(listener);
  rdma_destroy_event_channel(ec);
}

int main(int argc, char **argv)
{
  
  if (argc != 2) {
    printf("Invalid number of input arguments\n    Usage: %s <port>\n", argv[0]);
    exit(1);
  }
  
  rc_init(
    on_pre_conn,
    on_connection,
    on_completion,
    on_disconnect);

  printf("Server is listening...\n");

  rc_server_loop(argv[1]);

  return 0;
}
