#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>

struct message
{
  int id;
  union
  {
    struct
    {
      uint64_t addr;
      uint32_t rkey;
    } mr;
  } data;
};

enum message_id
{
  MSG_INVALID = 0,
  MSG_MR,
  MSG_READY,
  MSG_DONE
};

struct context {
  struct ibv_context *ctx;
  struct ibv_pd *pd;
  struct ibv_cq *cq;
  struct ibv_comp_channel *comp_channel;
  pthread_t cq_poll_thread;
};

struct server_context
{
  char *buffer;
  struct ibv_mr *buffer_mr;
  struct message *msg;
  struct ibv_mr *msg_mr;
};

struct client_context
{
  char *buffer;
  struct ibv_mr *buffer_mr;
  struct message *msg;
  struct ibv_mr *msg_mr;
  uint64_t peer_addr;
  uint32_t peer_rkey;
};

typedef void (*pre_conn_cb_fn)    (struct rdma_cm_id *id);
typedef void (*connect_cb_fn)     (struct rdma_cm_id *id);
typedef void (*completion_cb_fn)  (struct ibv_wc *wc);
typedef void (*disconnect_cb_fn)  (struct rdma_cm_id *id);

void rc_init(pre_conn_cb_fn, connect_cb_fn, completion_cb_fn, disconnect_cb_fn);
void event_loop(struct rdma_event_channel *ec, int exit_on_disconnect);
void build_params(struct rdma_conn_param *params);
struct ibv_pd * rc_get_pd();
void rc_server_loop(const char *port);