#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*	By default, RDMA_CM blocks until completion, which may not be ideal for
	this use case. These functions are non-blocking versions of rdma_get_*_comp
*/

static inline int
rdmae_poll_send_comp(struct rdma_cm_id *id, struct ibv_wc *wc)
{
	struct ibv_cq *cq;
	void *context;
	int ret;
	ret = ibv_poll_cq(id->send_cq, 1, wc);
	return (ret < 0) ? rdma_seterrno(ret) : ret;
}

static inline int
rdmae_poll_recv_comp(struct rdma_cm_id *id, struct ibv_wc *wc)
{
	struct ibv_cq *cq;
	void *context;
	int ret;
	ret = ibv_poll_cq(id->recv_cq, 1, wc);
	return (ret < 0) ? rdma_seterrno(ret) : ret;
}

/*  When performing RDMA Write, no notification is sent to the target of the
    write, which means polling the CQ will return no completion events.
    Performing write with immediate means the target can check whether this
    data was transferred or not by polling the CQ.
*/

static inline int
rdmae_post_writeimmv(struct rdma_cm_id *id, void *context, struct ibv_sge *sgl,
		int nsge, int flags, uint64_t remote_addr, uint32_t rkey,
		uint32_t imm_data)
{
	struct ibv_send_wr wr, *bad;
	wr.wr_id = (uintptr_t) context;
	wr.next = NULL;
	wr.sg_list = sgl;
	wr.num_sge = nsge;
	wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
	wr.send_flags = flags;
	wr.wr.rdma.remote_addr = remote_addr;
	wr.wr.rdma.rkey = rkey;
    wr.imm_data = imm_data;
	return rdma_seterrno(ibv_post_send(id->qp, &wr, &bad));
}

static inline int
rdmae_post_writeimm(struct rdma_cm_id *id, void *context, void *addr,
		size_t length, struct ibv_mr *mr, int flags,
		uint64_t remote_addr, uint32_t rkey, uint32_t imm_data)
{
	struct ibv_sge sge;
	sge.addr = (uint64_t) (uintptr_t) addr;
	sge.length = (uint32_t) length;
	sge.lkey = mr ? mr->lkey : 0;
	return rdma_post_writev(id, context, &sge, 1, flags, remote_addr, rkey);
}

/*	Simple function to extract immediate data in host byte order. */
static inline int
wc_ntohl(struct ibv_wc *wc)
{
	return ntohl(wc->imm_data);
}


#ifdef __cplusplus
}
#endif