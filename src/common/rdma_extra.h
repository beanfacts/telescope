#include <rdma/rdma_verbs.h>

static inline int
rdma_get_send_comp_solicited(struct rdma_cm_id *id, struct ibv_wc *wc)
{
	struct ibv_cq *cq;
	void *context;
	int ret;

	do {
		ret = ibv_poll_cq(id->send_cq, 1, wc);
		if (ret)
			break;

		ret = ibv_req_notify_cq(id->send_cq, 1);
		if (ret)
			return rdma_seterrno(ret);

		ret = ibv_poll_cq(id->send_cq, 1, wc);
		if (ret)
			break;

		ret = ibv_get_cq_event(id->send_cq_channel, &cq, &context);
		if (ret)
			return ret;

		assert(cq == id->send_cq && context == id);
		ibv_ack_cq_events(id->send_cq, 1);
	} while (1);

	return (ret < 0) ? rdma_seterrno(ret) : ret;
}