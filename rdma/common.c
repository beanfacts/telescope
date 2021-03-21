#include "common.h"

const int RESOLVE_TIMEOUT = 1000;

static struct context *s_ctx = NULL;

static pre_conn_cb_fn s_on_pre_conn_cb = NULL;
static connect_cb_fn s_on_connect_cb = NULL;
static completion_cb_fn s_on_completion_cb = NULL;
static disconnect_cb_fn s_on_disconnect_cb = NULL;

static void build_context(struct ibv_context *verbs);
static void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
static void * poll_cq(void *);

void build_connection(struct rdma_cm_id *id)
{
  struct ibv_qp_init_attr qp_attr;

  build_context(id->verbs);
  build_qp_attr(&qp_attr);

  if (rdma_create_qp(id, s_ctx->pd, &qp_attr)) {
    printf("Could not allocate QP\n");
  };
}

void build_context(struct ibv_context *verbs)
{
  
  if (s_ctx) {

    if (s_ctx->ctx != verbs) {
      printf("Cannot handle multiple contexts\n");
      exit(1);
    }
    return;
  }

  s_ctx = (struct context *)malloc(sizeof(struct context));

  s_ctx->ctx = verbs;

  s_ctx->pd = ibv_alloc_pd(s_ctx->ctx);
  if (!s_ctx->pd) {
    printf("Could not allocate a protection domain\n");
    exit(1);
  }

  s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx);
  if (!s_ctx->comp_channel) {
    printf("Could not allocate ibv completion channel\n");
    exit(1);
  }

  s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0);
  if (!s_ctx->cq) {
    printf("Could not allocate completion queue\n");
    exit(1);
  }

  if (ibv_req_notify_cq(s_ctx->cq, 0)) {
    printf("Error requesting a completion notification\n");
    exit(1);
  }

  if (pthread_create(&s_ctx->cq_poll_thread, NULL, poll_cq, NULL)) {
    printf("Could not create polling thread\n");
    exit(1);
  };
}

void build_params(struct rdma_conn_param *params)
{
  memset(params, 0, sizeof(*params));
  params->initiator_depth = params->responder_resources = 1;
  params->rnr_retry_count = 7;
}

void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
{
  memset(qp_attr, 0, sizeof(*qp_attr));

  qp_attr->send_cq = s_ctx->cq;
  qp_attr->recv_cq = s_ctx->cq;
  qp_attr->qp_type = IBV_QPT_RC;

  qp_attr->cap.max_send_wr = 10;
  qp_attr->cap.max_recv_wr = 10;
  qp_attr->cap.max_send_sge = 1;
  qp_attr->cap.max_recv_sge = 1;
}

extern void event_loop(struct rdma_event_channel *ec, int exit_on_disconnect)
{
  
  int ret;
  struct rdma_cm_event *event = NULL;
  struct rdma_conn_param cm_params;

  build_params(&cm_params);

  while (rdma_get_cm_event(ec, &event) == 0) {
    
    struct rdma_cm_event event_copy;

    memcpy(&event_copy, event, sizeof(*event));
    rdma_ack_cm_event(event);

    if (event_copy.event == RDMA_CM_EVENT_ADDR_RESOLVED) {
      
      build_connection(event_copy.id);

      if (s_on_pre_conn_cb)
        s_on_pre_conn_cb(event_copy.id);

      ret = rdma_resolve_route(event_copy.id, RESOLVE_TIMEOUT);
      if (ret == -1) {
        printf("Could not resolve route to host\n");
      }

    } else if (event_copy.event == RDMA_CM_EVENT_ROUTE_RESOLVED) {
      if (rdma_connect(event_copy.id, &cm_params)) {
        printf("Could not establish an RDMA connection\n");
      };

    } else if (event_copy.event == RDMA_CM_EVENT_CONNECT_REQUEST) {
      build_connection(event_copy.id);

      if (s_on_pre_conn_cb)
        s_on_pre_conn_cb(event_copy.id);

      if (rdma_accept(event_copy.id, &cm_params)) {
        printf("Could not accept RDMA connection.\n");
      };

    } else if (event_copy.event == RDMA_CM_EVENT_ESTABLISHED) {
      if (s_on_connect_cb)
        s_on_connect_cb(event_copy.id);

    } else if (event_copy.event == RDMA_CM_EVENT_DISCONNECTED) {
      rdma_destroy_qp(event_copy.id);

      if (s_on_disconnect_cb)
        s_on_disconnect_cb(event_copy.id);

      rdma_destroy_id(event_copy.id);

      if (exit_on_disconnect)
        break;

    } else {
      printf("Unknown error occurred\n");
      exit(1);
    }
  }
}

void * poll_cq(void *ctx)
{
  struct ibv_cq *cq;
  struct ibv_wc wc;

  while (1) {
    if(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx)) {
      printf("Could not get completion queue event.\n");
    };
    
    ibv_ack_cq_events(cq, 1);
    if(ibv_req_notify_cq(cq, 0)) {
      printf("Could not notify CQ event.\n");
    };

    while (ibv_poll_cq(cq, 1, &wc)) {
      if (wc.status == IBV_WC_SUCCESS)
        s_on_completion_cb(&wc);
      else {
        printf("Unknown CQ poll status\n");
      }
    }
  }

  return NULL;
}

void rc_init(pre_conn_cb_fn pc, connect_cb_fn conn, completion_cb_fn comp, disconnect_cb_fn disc)
{
  s_on_pre_conn_cb = pc;
  s_on_connect_cb = conn;
  s_on_completion_cb = comp;
  s_on_disconnect_cb = disc;
}

struct ibv_pd * rc_get_pd()
{
  return s_ctx->pd;
}