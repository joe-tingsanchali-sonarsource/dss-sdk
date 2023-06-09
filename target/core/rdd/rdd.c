/**
 *   BSD LICENSE
 *
 *   Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Samsung Electronics Co., Ltd. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "rdd.h"

#include <time.h>

//MAX_RDD_SEG_SIZE 8MB
#define MAX_RDD_SEG_SIZE (0x800000)
#define MAX_LISTENER_NAME (256)


//Client Handle framework APIs
//
static inline uint16_t RDD_CHANDLE2INDEX(rdd_ctx_t *ctx, uint16_t chandle) {return (ctx->handle_ctx.hmask ^ chandle);}
static inline uint16_t RDD_INVALID_CHANDLE(rdd_ctx_t *ctx) { return RDD_CHANDLE2INDEX(ctx, RDD_MAX_CHANDLE_CNT);}

uint32_t  rdd_dss_check_submit_ring(struct rdd_rdma_queue_s *queue);

uint16_t rdd_reg_queue2ctx(rdd_ctx_t *ctx, struct rdd_rdma_queue_s *queue)
{
	uint16_t id = RDD_INVALID_CHANDLE(ctx);;

	DFLY_ASSERT(ctx == queue->ctx);

	pthread_rwlock_wrlock(&ctx->handle_ctx.rwlock);

	if(ctx->handle_ctx.nuse == RDD_MAX_CHANDLE_CNT) {
		goto err;
	}

	//TODO: Reuse deregistered handles
	//Loop through till nuse and find if any free handle??
	//If found go to allocation logic.. Use another index var??

	if(ctx->handle_ctx.nuse == ctx->handle_ctx.nhandles) {
		ctx->handle_ctx.handle_arr = realloc(ctx->handle_ctx.handle_arr, 2 *  ctx->handle_ctx.nhandles * sizeof(ctx->handle_ctx.handle_arr));
		DFLY_ASSERT(ctx->handle_ctx.handle_arr);
		//TODO: set NULL for new elements

		ctx->handle_ctx.nhandles *= 2;//Double memory
		DFLY_NOTICELOG("handle count expanded to %d\n", ctx->handle_ctx.nhandles);
	}


	queue->ref = dss_ref_create();;
    if(!queue->ref) {
        goto err;
    }

	id = ctx->handle_ctx.hmask ^ ctx->handle_ctx.nuse;
	ctx->handle_ctx.handle_arr[ctx->handle_ctx.nuse] = queue;
	ctx->handle_ctx.nuse++;

err:
	pthread_rwlock_unlock(&ctx->handle_ctx.rwlock);

	return id;
}

bool rdd_check_qref(rdd_ctx_t *ctx, uint16_t client_handle)
{
	uint16_t index = RDD_CHANDLE2INDEX(ctx, client_handle);
    struct rdd_rdma_queue_s *q;

	DFLY_ASSERT(client_handle != RDD_INVALID_CHANDLE(ctx));
    q = ctx->handle_ctx.handle_arr[index];
    if(!q) {
        return false;
    }
    return dss_ref_check(q->ref);
}

void rdd_freeze_queue_ctx(rdd_ctx_t *ctx, uint16_t client_handle)
{
	uint16_t index = RDD_CHANDLE2INDEX(ctx, client_handle);
    struct rdd_rdma_queue_s *q;

	DFLY_ASSERT(client_handle != RDD_INVALID_CHANDLE(ctx));

	//TODO: Reclaim unused handles
	pthread_rwlock_wrlock(&ctx->handle_ctx.rwlock);

	DFLY_ASSERT(ctx->handle_ctx.handle_arr[index] != NULL);
    q = ctx->handle_ctx.handle_arr[index];
    if(!q) {
        DFLY_ASSERT(q != NULL);
        return;
    }
    dss_ref_freeze(q->ref);

	pthread_rwlock_unlock(&ctx->handle_ctx.rwlock);

    return;
}


void rdd_del_queue_ctx(rdd_ctx_t *ctx, uint16_t client_handle)
{

	uint16_t index = RDD_CHANDLE2INDEX(ctx, client_handle);
    struct rdd_rdma_queue_s *q;

	DFLY_ASSERT(client_handle != RDD_INVALID_CHANDLE(ctx));

	//TODO: Reclaim unused handles
	pthread_rwlock_wrlock(&ctx->handle_ctx.rwlock);

	DFLY_ASSERT(ctx->handle_ctx.handle_arr[index] != NULL);
    q = ctx->handle_ctx.handle_arr[index];
    if(!q) {
        DFLY_ASSERT(q != NULL);
        return;
    }
    dss_ref_destroy(q->ref);
    q->ref = NULL;
	ctx->handle_ctx.handle_arr[index] = NULL;

	DFLY_NOTICELOG("Deleted client handle %d\n", index);

	pthread_rwlock_unlock(&ctx->handle_ctx.rwlock);

	return;
}

int rdd_post_req2queue(rdd_ctx_t *ctx, uint16_t client_handle, dfly_request_t *req)
{
	int rc = 0;
	uint16_t index = RDD_CHANDLE2INDEX(ctx, client_handle);

    struct dss_ref_s *ref;

	//uint64_t start_tick = 0;
	//uint64_t ticks_per_ns = spdk_get_ticks_hz()/1000000000;

	if(client_handle == RDD_INVALID_CHANDLE(ctx)) {
        DFLY_ERRLOG("Failed to find valid chandle %x\n", client_handle);
		return -1;
	}

	//start_tick = spdk_get_ticks();

	//pthread_rwlock_rdlock(&ctx->handle_ctx.rwlock);

	if(index >= ctx->handle_ctx.nuse) {
		rc = -1;
        DFLY_ERRLOG("RDD handle index not used [%d/%d]\n", index, ctx->handle_ctx.nuse);
		goto last;
	}

	req->rdd_info.q = ctx->handle_ctx.handle_arr[index];

	if(req->rdd_info.q == NULL) {
		rc = -1;
        DFLY_ERRLOG("RDD handle is NULL at index %d\n", index);
		goto last;
	}

    ref = req->rdd_info.q->ref;;
    if(!ref) {
        rc = -1;
        goto last;
    }

    if (!dss_ref_inc(ref)) {
        rc = -1;
        goto last;
    }

	
    //TODO: Check if there can be a race here to get index while disconnecting
	//Other rdd info needs to be filled prior to call

	rdd_dss_submit_request((void *)req);

    dss_ref_dec(ref);

	//req->rdd_info.q->submit_latency += ((spdk_get_ticks() - start_tick)/ticks_per_ns);
	//req->rdd_info.q->submit_count++;

last:
	//pthread_rwlock_unlock(&ctx->handle_ctx.rwlock);

   //if(rc) {
   //   DFLY_ERRLOG("Failed to post to rdd queue %p\n", q);
   //}

	return rc;
}

//End - Client Handle framework APIs
//

static int
rdd_rdma_mem_notify(void *cb_ctx, struct spdk_mem_map *map,
		     enum spdk_mem_map_notify_action action,
		     void *vaddr, size_t size)
{
	struct ibv_pd *pd = cb_ctx;
	struct ibv_mr *mr;
	int rc = -1;

	switch (action) {
	case SPDK_MEM_MAP_NOTIFY_REGISTER:
			mr = ibv_reg_mr(pd, vaddr, size,
					IBV_ACCESS_LOCAL_WRITE |
					IBV_ACCESS_REMOTE_READ |
					IBV_ACCESS_REMOTE_WRITE);
			if (mr == NULL) {
				DFLY_ERRLOG("ibv_reg_mr() failed\n");
				return -1;
			} else {
				rc = spdk_mem_map_set_translation(map, (uint64_t)vaddr, size, (uint64_t)mr);
			}
		break;
	case SPDK_MEM_MAP_NOTIFY_UNREGISTER:
		mr = (struct ibv_mr *)spdk_mem_map_translate(map, (uint64_t)vaddr, NULL);
		if (mr) {
			ibv_dereg_mr(mr);
		}
		rc = spdk_mem_map_clear_translation(map, (uint64_t)vaddr, size);
		break;
	default:
        abort();
	}

	return rc;
}

static int
rdd_rdma_check_contiguous_entries(uint64_t addr_1, uint64_t addr_2)
{
	/* Two contiguous mappings will point to the same address which is the start of the RDMA MR. */
	return addr_1 == addr_2;
}

const struct spdk_mem_map_ops g_rdd_rdma_map_ops = {
	.notify_cb = rdd_rdma_mem_notify,
	.are_contiguous = rdd_rdma_check_contiguous_entries
};

static int qc = 0;

int rdd_cl_queue_established(struct rdd_rdma_listener_s  *l, struct rdma_cm_event *ev)
{
    struct rdma_cm_id *id = ev->id;
    struct rdd_rdma_queue_s *queue = (struct rdd_rdma_queue_s *)id->context;

    //DFLY_NOTICELOG("qid %u connection estalished\n", queue->host_qid);

    pthread_mutex_lock(&queue->init_lock);
    //TODO: Handle disconnecting;
    if(queue->state == RDD_QUEUE_CONNECTING) {
        queue->state = RDD_QUEUE_ESTABLISHED;
    } else if (queue->state == RDD_QUEUE_READY) {
        queue->state = RDD_QUEUE_LIVE;
    }

    pthread_mutex_unlock(&queue->init_lock);

    TAILQ_INSERT_TAIL(&l->queues, queue, link);

    qc++;
    if(qc % 25 == 0) {
        DFLY_NOTICELOG("Established %d queues\n", qc);
    }

    return 0;
}

void rdd_dss_flush_n_fail_reqs(struct rdd_rdma_queue_s *q)
{
    dfly_request_t *req, *req_tmp;
    TAILQ_FOREACH_SAFE(req, &q->pending_reqs, rdd_info.pending, req_tmp) {
        TAILQ_REMOVE(&q->pending_reqs, req, rdd_info.pending);
        dss_rdma_rdd_failed(req, NULL);
    }
}

int rdd_cq_shutdown(void *arg)
{
    struct rdd_rdma_queue_s *q = (struct rdd_rdma_queue_s *)arg;
    uint64_t tick_now = spdk_get_ticks();
    uint64_t end_tick = (spdk_get_ticks_hz()/1000000) * RDD_DEFAULT_CQ_SHUTDOWN_TIMEOUT_IN_US;

    end_tick += q->th.spdk.shutdown_start_tick;

    if(tick_now < end_tick) {
        return SPDK_POLLER_BUSY;
    }

    if(rdd_check_qref(q->ctx, q->qhandle)) {
        return SPDK_POLLER_BUSY;
    }

    spdk_poller_unregister(&q->th.spdk.shutdown_poller);

    rdd_del_queue_ctx(q->ctx, q->qhandle);

    while(rdd_dss_check_submit_ring(q));

    spdk_ring_free(q->req_submit_ring);
    q->req_submit_ring = NULL;

    rdd_dss_flush_n_fail_reqs(q);

    rdd_queue_destroy(q);

    DFLY_NOTICELOG("queue %p destroyed\n", q);
    return SPDK_POLLER_IDLE;
}

static void __rdd_cl_queue_disconnect(void *arg)
{
    struct rdd_rdma_queue_s *q = (struct rdd_rdma_queue_s *)arg;

    rdd_freeze_queue_ctx(q->ctx, q->qhandle);//stop incoming post requests

    //TODO: Check all queue states
    q->state = RDD_QUEUE_DISCONNECTING;

    spdk_poller_unregister(&q->th.spdk.cq_poller);

    q->th.spdk.shutdown_poller = SPDK_POLLER_REGISTER(rdd_cq_shutdown, q, RDD_DEFAULT_CQ_SHUTDOWN_TIMEOUT_IN_US);
    q->th.spdk.shutdown_start_tick = spdk_get_ticks();

    return;
}

int rdd_cl_queue_disconnect(struct rdd_rdma_listener_s  *l, struct rdma_cm_event *ev)
{
    struct rdma_cm_id *id = ev->id;
    struct rdd_rdma_queue_s *queue = (struct rdd_rdma_queue_s *)id->context;


    DFLY_NOTICELOG("qid %u listen [%s] peer [%s] disconnect receieved\n", queue->host_qid, l->listen_ip, queue->peer_ip);

	uint64_t avg_sub_latency;

    if(queue->submit_count) {
        avg_sub_latency = (queue->submit_latency/queue->submit_count);
        DFLY_NOTICELOG("Average submit latency l_ip[%s] p_ip[%s] %lu/%lu =%lu\n", l->listen_ip, queue->peer_ip, queue->submit_latency, queue->submit_count, avg_sub_latency);
    }

    spdk_thread_send_msg(queue->th.spdk.cq_thread, __rdd_cl_queue_disconnect, queue);
    //queue->state = RDD_QUEUE_LIVE;

    //TAILQ_INSERT_TAIL(&ctx->queues, queue, link);
//TODO: Free resource on disconnect
//    spdk_poller_unregister(queue->th.spdk.cq_poller);
//	queue->th.spdk.cq_poller = NULL;

//	if (queue->req_submit_ring != NULL) {
//		spdk_ring_free(queue->req_submit_ring);
//		queue->req_submit_ring = NULL;
//	}

    return 0;
}

struct rdd_req_s *rdd_get_free_request(struct rdd_rdma_queue_s *q)
{
    struct rdd_req_s *req;

    if(TAILQ_EMPTY(&q->free_reqs)) {
        return NULL;
    }

    req = TAILQ_FIRST(&q->free_reqs);
    TAILQ_REMOVE(&q->free_reqs, req, link);

    TAILQ_INSERT_TAIL(&q->outstanding_reqs, req, link);

	q->outstanding_qd++;

    return req;
}

void rdd_put_free_request(struct rdd_req_s *req)
{
    DFLY_ASSERT(req != NULL);

    TAILQ_REMOVE(&req->q->outstanding_reqs, req, link);

    //Temporary freeing of source address
    //TODO: This should be done by the one submitting the request??
    //spdk_free((void *)req->rdd_cmd->cmd.hread.caddr);

	req->q->submit_latency += ((spdk_get_ticks() - req->start_tick)/(spdk_get_ticks_hz()/1000000));
	req->q->submit_count++;

    req->ctx = NULL;

    TAILQ_INSERT_TAIL(&req->q->free_reqs, req, link);

	if(req->rdd_cmd->opc == RDD_CMD_HOST_READ) {
    	ibv_post_recv(req->q->qp, &req->q->rsps[req->rsp_idx].recv_wr, NULL);
	}
	req->q->outstanding_qd--;

    return;
}

//Run in same thread as the queue is polled
int rdd_post_cmd_host_read(struct rdd_rdma_queue_s *q, uint64_t cuid, void *cmem, void *hmem, uint32_t vlen, void *ctx)
{
    int rc;
    //int translation_len = 0;
    struct rdd_req_s *req;

    struct ibv_mr *data_mr = NULL;
    struct ibv_send_wr *bad_wr = NULL;

    if(vlen > RDD_RDMA_MAX_KEYED_SGL_LENGTH) {
        DFLY_NOTICELOG("Unsupported value length\n");
        return -1;
    }

    req = rdd_get_free_request(q);
    if(!req) {
        assert(0);
        return -1;
    }

	req->ctx = ctx;
    req->cuid = cuid;
    
    //TODO: Update command
    req->rdd_cmd->opc = RDD_CMD_HOST_READ;
    req->rdd_cmd->cmd.hread.len = vlen;
    req->rdd_cmd->cmd.hread.caddr = (uint64_t) cmem;
    data_mr = (struct ibv_mr *)spdk_mem_map_translate(q->map, (uint64_t)cmem, NULL);
    req->rdd_cmd->cmd.hread.ckey = data_mr->rkey;
    //DFLY_ASSERT(translation_len >= vlen);
    req->rdd_cmd->cmd.hread.haddr = (uint64_t) hmem;// TODO: from host

	req->start_tick = spdk_get_ticks();

    //DFLY_NOTICELOG("Sending Command\n");
    rc = ibv_post_send(q->qp, &req->req.send_wr, &bad_wr);
    assert(rc ==0);
    assert(bad_wr == NULL);
    //TODO: Handle error??

    return rc;
}

static int rdd_update_sgl_payload(struct rdd_req_s *r, uint32_t k, void *m, uint32_t l, uint32_t max_segs)
{
    int rc = 0, count = 0;
    int s =0;
    while(l) {
        if(l < MAX_RDD_SEG_SIZE) {
            s = l;
        } else {
            //Set Max Size
            s = MAX_RDD_SEG_SIZE;
        }
        r->data.data_sge[count].addr = m;
        r->data.data_sge[count].length = s;
        r->data.data_sge[count].lkey = k;
        m += s;
        l -= s;
        count++;
        if(count >= max_segs) {
            break;
        }
    }
    //Set num sge
    r->data.data_wr.num_sge = count;

    if(l) rc = -1;

    return rc;
}

//Run in same thread as the queue is polled
int rdd_post_cmd_ctrl_write(struct rdd_rdma_queue_s *q, uint64_t cuid, void *cmem, void *hmem, uint32_t hkey, uint32_t vlen, void *ctx)
{
    int rc;
    //int translation_len = 0;
    struct rdd_req_s *req;

    struct ibv_mr *data_mr = NULL;
    struct ibv_send_wr *bad_wr = NULL;

    if(vlen > (q->ctx->max_sgl_segs * MAX_RDD_SEG_SIZE)) {
        DFLY_NOTICELOG("Unsupported value length %d\n", vlen);
        return -1;
    }

    req = rdd_get_free_request(q);
    if(!req) {
        assert(0);
        return -1;
    }

	req->ctx = ctx;
    req->cuid = cuid;
    
    //TODO: Update command
    req->rdd_cmd->opc = RDD_CMD_CTRL_WRITE;
    req->data.rdd_wr.type    = RDD_WR_TYPE_DATA_WRITE;
    req->data.data_wr.opcode = IBV_WR_RDMA_WRITE;

       req->data.data_wr.wr.rdma.remote_addr = hmem;
       req->data.data_wr.wr.rdma.rkey = hkey;
    data_mr = (struct ibv_mr *)spdk_mem_map_translate(q->map, (uint64_t)cmem, NULL);

    rc = rdd_update_sgl_payload(req, data_mr->lkey, cmem, vlen, q->ctx->max_sgl_segs);
    if(rc) {
        return rc;
    }
       req->start_tick = spdk_get_ticks();

    //DFLY_NOTICELOG("Sending Command\n");
    rc = ibv_post_send(q->qp, &req->data.data_wr, &bad_wr);
    assert(rc ==0);
    assert(bad_wr == NULL);
    //TODO: Handle error??

    return rc;
}


int rdd_post_cmd_ctrl_read(struct rdd_rdma_queue_s *q, uint64_t cuid, void *cmem, void *hmem, uint32_t hkey, uint32_t vlen, void *ctx)
{
    int rc;
    //int translation_len = 0;
    struct rdd_req_s *req;

    struct ibv_mr *data_mr = NULL;
    struct ibv_send_wr *bad_wr = NULL;

    if(vlen > (q->ctx->max_sgl_segs * MAX_RDD_SEG_SIZE)) {
        DFLY_NOTICELOG("Unsupported value length %d\n", vlen);
        return -1;
    }

    req = rdd_get_free_request(q);
    if(!req) {
        assert(0);
        return -1;
    }

    req->ctx = ctx;
    req->cuid = cuid;

    //TODO: Update command
    req->rdd_cmd->opc = RDD_CMD_CTRL_READ;

    req->data.rdd_wr.type    = RDD_WR_TYPE_DATA_READ;
    req->data.data_wr.opcode = IBV_WR_RDMA_READ;

	req->data.data_wr.wr.rdma.remote_addr = hmem;
	req->data.data_wr.wr.rdma.rkey = hkey;
    data_mr = (struct ibv_mr *)spdk_mem_map_translate(q->map, (uint64_t)cmem, NULL);

    rc = rdd_update_sgl_payload(req, data_mr->lkey, cmem, vlen, q->ctx->max_sgl_segs);
    if(rc) {
        return rc;
    }

	req->start_tick = spdk_get_ticks();

    //DFLY_NOTICELOG("Sending Command\n");
    rc = ibv_post_send(q->qp, &req->data.data_wr, &bad_wr);
    assert(rc ==0);
    assert(bad_wr == NULL);
    //TODO: Handle error??

    return rc;
}

void rdd_dss_submit_request(void * arg)
{
	dfly_request_t *req = (dfly_request_t *)arg;
	int rc;

	struct rdd_rdma_queue_s *q = req->rdd_info.q;

	DFLY_DEBUGLOG(DSS_RDD, "Queued data direct request for dss req %p\n", req);

    if(q->state == RDD_QUEUE_CONNECTING ||
        q->state == RDD_QUEUE_ESTABLISHED) {//For any other state submit ring should be initialized
        //Submission path uses lock only during initialization
        pthread_mutex_lock(&q->init_lock);
        //Check again after acquiring lock
        if(q->state == RDD_QUEUE_CONNECTING || q->state == RDD_QUEUE_ESTABLISHED) {
            TAILQ_INSERT_TAIL(&q->pending_reqs, req, rdd_info.pending);
            //Unlock after inseting to pending request queue
            pthread_mutex_unlock(&q->init_lock);
            return;
        }
        //Unlock for q state changed after lock
        pthread_mutex_unlock(&q->init_lock);

    }

    DFLY_ASSERT(q->req_submit_ring);
    rc = spdk_ring_enqueue(q->req_submit_ring, (void **)&req, 1, NULL);
    if (rc != 1) {
        assert(false);
    }

	return;
}

void rdd_dss_complete(struct rdd_req_s *rdd_req) {

	dfly_request_t *req = (dfly_request_t *)rdd_req->ctx;
	//TODO: Process dss req to send for completion
	
	if(req->data_direct == true) {
//#ifndef RDD_TEST
		DFLY_DEBUGLOG(DSS_RDD, "Direct data transfer completed for request %p\n", req);
		dss_rdma_rdd_complete(rdd_req->ctx, NULL);	
//#endif
	} else {
		//TMP free allocated req
		free(rdd_req->ctx);
	}
	return;
}

int rdd_process_resp(struct rdd_req_s *req)
{
	rdd_dss_complete(req);
    return 0;
}

void rdd_dss_process_nvmf(struct rdd_req_s *rdd_req) {
       dfly_request_t *req = (dfly_request_t *)rdd_req->ctx;

       DFLY_DEBUGLOG(DSS_RDD, "Direct data transfer h2c completed for request %p\n", req);
       dss_rdma_rdd_in_data_ready(rdd_req->ctx, NULL);

       return;
}

static inline bool rdd_check_cuid(struct rdd_req_s *r)
{
    dfly_request_t *dreq;

    dreq = (dfly_request_t *)r->ctx;

    if(dreq->rdd_info.req_cuid == r->cuid) {
        return true;
    } else {
        DFLY_WARNLOG("Dropping rdd request not matching cuid %x\n", r);
        return false;
    }
}
int rdd_process_wc(struct ibv_wc *wc)
{
    struct rdd_req_s *req = NULL;
    struct rdd_rsp_s *rsp;
    struct rdd_wr_s *rdd_wr = (struct rdd_wr_s *)wc->wr_id;
    int rc = -1;

    //DFLY_NOTICELOG("Got wc with opcode %d and status %s\n", wc->opcode, ibv_wc_status_str(wc->status));

    if(wc->status != IBV_WC_SUCCESS) {
        DFLY_ERRLOG("wc error %d\n", wc->status);
        switch(rdd_wr->type) {
            case RDD_WR_TYPE_REQ_SEND:
                //TODO: Host recieved SEND??
                //DFLY_NOTICELOG("Send acknowledgement recieved\n");
                DFLY_NOTICELOG("WC %d type recieved\n", RDD_WR_TYPE_REQ_SEND);
                break;
            case RDD_WR_TYPE_DATA_READ:
                req = SPDK_CONTAINEROF(rdd_wr, struct rdd_req_s, data);
                rc = 0;
                break;
            case RDD_WR_TYPE_DATA_WRITE:
                req = SPDK_CONTAINEROF(rdd_wr, struct rdd_req_s, data);
                rc = 0;
                break;
            case RDD_WR_TYPE_RSP_RECV:
                rsp = SPDK_CONTAINEROF(rdd_wr, struct rdd_rsp_s, rdd_wr);
                req = &rsp->q->reqs[rsp->rsp.cid];
                rc = 0;
                break;
            default:
                DFLY_NOTICELOG("Unknown WC type recieved\n");
                break;
        }
        if(req && req->ctx) {
            if(rdd_check_cuid(req)) {
                dss_rdma_rdd_failed((dfly_request_t *)req->ctx, NULL);
            }
            rdd_put_free_request(req);
        } //else already freed?
        return rc;
    }

    switch(rdd_wr->type) {
        case RDD_WR_TYPE_REQ_SEND:
            //TODO: Host recieved SEND??
            //DFLY_NOTICELOG("Send acknowledgement recieved\n");
            break;
        case RDD_WR_TYPE_DATA_READ:
            req = SPDK_CONTAINEROF(rdd_wr, struct rdd_req_s, data);
            //TODO: Process nvmf command PUT should write data now
            //rdd_process_resp(req);
            if(rdd_check_cuid(req)) {
                rdd_dss_process_nvmf(req);
            }
            rdd_put_free_request(req);
            break;
        case RDD_WR_TYPE_DATA_WRITE:
            req = SPDK_CONTAINEROF(rdd_wr, struct rdd_req_s, data);
            if(rdd_check_cuid(req)) {
			    rdd_process_resp(req);
            }
            rdd_put_free_request(req);
			break;
        case RDD_WR_TYPE_RSP_RECV:
            rsp = SPDK_CONTAINEROF(rdd_wr, struct rdd_rsp_s, rdd_wr);
            req = &rsp->q->reqs[rsp->rsp.cid];
            req->rsp_idx = rsp->idx;

            //DFLY_NOTICELOG("comand completion recieved opc %d cid %d status %d\n", req->rdd_cmd->opc, rsp->rsp.cid, rsp->rsp.status);
            if(rdd_check_cuid(req)) {
			    rdd_process_resp(req);
            }
            rdd_put_free_request(req);
            //ibv_post_recv(rsp->q->qp, &rsp->recv_wr, NULL);
            //TODO: Handle reture code
            break;
        default:
            DFLY_NOTICELOG("Unknown WC type recieved\n");
            break;
    }

    return 0;
}

#define REQ_PER_POLL (8)

uint32_t  rdd_dss_check_submit_ring(struct rdd_rdma_queue_s *queue)
{
	int num_msgs, i;
	void *reqs[REQ_PER_POLL] = {NULL};

	num_msgs = spdk_ring_dequeue(queue->req_submit_ring, reqs, REQ_PER_POLL);
	for (i = 0; i < num_msgs; i++) {
    	TAILQ_INSERT_TAIL(&queue->pending_reqs, (dfly_request_t *)reqs[i], rdd_info.pending);
	}

    return num_msgs;
}

static inline uint64_t dss_gen_cuid(rdd_ctx_t *ctx)
{
    return __atomic_fetch_add(&ctx->cuid, 1, __ATOMIC_RELAXED);
}

void rdd_dss_process_pending(struct rdd_rdma_queue_s *queue)
{
	dfly_request_t *req, *req_tmp;
	int rc;
    int reqs_processed = 0;


	TAILQ_FOREACH_SAFE(req, &queue->pending_reqs, rdd_info.pending, req_tmp) {
		if(queue->outstanding_qd == queue->send_qd) {
			break;//Queue is full
		}
    	TAILQ_REMOVE(&queue->pending_reqs, req, rdd_info.pending);

        req->rdd_info.req_cuid = dss_gen_cuid(queue->ctx);

		switch(req->rdd_info.opc) {
			case RDD_CMD_HOST_READ:
				//DFLY_NOTICELOG("opc %d, cmem %p hmem %p len %d\n", req->rdd_info.opc,
				//					(void *)req->rdd_info.cmem, (void *) req->rdd_info.hmem,
				//						req->rdd_info.payload_len);
				DFLY_DEBUGLOG(DSS_RDD, "Initiate direct data transfer for request %p\n", req);
				rc = rdd_post_cmd_host_read(queue, req->rdd_info.req_cuid, (void *)req->rdd_info.cmem,
												(void *) req->rdd_info.hmem,
												req->rdd_info.payload_len,
												(void *)req);
				DFLY_ASSERT(rc == 0);
				break;
			case RDD_CMD_CTRL_WRITE:
				DFLY_DEBUGLOG(DSS_RDD, "Initiate direct data transfer ctrl write req %p\n", req);
				rc = rdd_post_cmd_ctrl_write(queue, req->rdd_info.req_cuid, (void *)req->rdd_info.cmem,
												(void *) req->rdd_info.hmem,
												(uint32_t)req->rdd_info.hkey,
												req->rdd_info.payload_len,
												(void *)req);
				DFLY_ASSERT(rc == 0);
				break;
             case RDD_CMD_CTRL_READ:
                DFLY_DEBUGLOG(DSS_RDD, "Initiate direct data transfer ctrl read req %p\n", req);
                rc = rdd_post_cmd_ctrl_read(queue, req->rdd_info.req_cuid, (void *)req->rdd_info.cmem,
                                                (void *) req->rdd_info.hmem,
                                                (uint32_t)req->rdd_info.hkey,
                                                req->rdd_info.payload_len,
                                                (void *)req);
                DFLY_ASSERT(rc == 0);
                break;
			default:
				DFLY_NOTICELOG("Unhandled opc %d\n", req->rdd_info.opc);
		}
        reqs_processed++;
        if(reqs_processed >= REQ_PER_POLL) {
            break;
        }
	}

	return;
}

int rdd_cq_poll(void *arg)
{
    struct rdd_rdma_queue_s *queue = (struct rdd_rdma_queue_s *)arg;
    //struct ibv_comp_channel * ch = queue->comp_ch;

    struct ibv_cq *cq;
    struct ibv_wc wc;
    //void *ctx;

    int rc;

    //Check ring for pending requests
    uint32_t  nreqs;
    nreqs = rdd_dss_check_submit_ring(queue);
    //TODO: Set POLLER BUSY??

    //Process pending reqests
    rdd_dss_process_pending(queue);
	

    //ibv_get_cq_event(ch, &cq, &ctx);
    //ibv_ack_cq_events(cq, 1);
    //ibv_req_notify_cq(cq, 0);

    //TODO: Handle multiple wc in one poll
	if ((rc = ibv_poll_cq(queue->cq, 1, &wc)) > 0) {
        rc = rdd_process_wc(&wc);
        if(rc) {
            //Error processing WC
            //TODO: Handle error
        }
        return SPDK_POLLER_BUSY;
  	} else {
        if(rc <0) {
            DFLY_ERRLOG("Rdma cq poll failed for %p\n", queue);
            //TODO: Error handling
        }
    }
	return SPDK_POLLER_IDLE;
}

static void _rdd_start_cq_poller(void *arg)
{
    struct rdd_rdma_queue_s *queue = (struct rdd_rdma_queue_s *)arg;

    DFLY_ASSERT(spdk_get_thread() == queue->th.spdk.cq_thread);
    DFLY_ASSERT(queue->th.spdk.cq_poller == NULL);

    if(queue->state == RDD_QUEUE_DISCONNECTING) {
        DFLY_WARNLOG("Queue %p in disconnecting state\n", queue);
        return;
    }

    queue->req_submit_ring = spdk_ring_create(SPDK_RING_TYPE_MP_SC, 65536, SPDK_ENV_SOCKET_ID_ANY);
    assert(queue->req_submit_ring);

    pthread_mutex_lock(&queue->init_lock);

    if(queue->state == RDD_QUEUE_CONNECTING) {
        queue->state = RDD_QUEUE_READY;
    } else if (queue->state == RDD_QUEUE_ESTABLISHED) {
        queue->state = RDD_QUEUE_LIVE;
    }

    rdd_dss_process_pending(queue);

    pthread_mutex_unlock(&queue->init_lock);

    queue->th.spdk.cq_poller = SPDK_POLLER_REGISTER(rdd_cq_poll, queue, 0);
    DFLY_ASSERT(queue->th.spdk.cq_poller != NULL);

    //DFLY_NOTICELOG("Started cq poll thread for queue %p on core %d \n", queue, spdk_env_get_current_core());

    return;
}

//TODO: Run on core where the ib queue needs to be polled
static int rdd_queue_ib_create(struct rdd_rdma_queue_s *queue)
{
    struct ibv_cq *cq;
    struct ibv_qp_init_attr qp_attr;
    struct rdma_cm_id *id = queue->cm_id;

    int cqc;
    char thread_name[MAX_LISTENER_NAME] = {0};
    int rc = 0;
    int i;

    uint32_t icore;
    char *rdd_conn_id = NULL;
    struct spdk_cpuset cset;

    DFLY_ASSERT(queue->comp_ch == NULL);
    queue->comp_ch = ibv_create_comp_channel(id->verbs);

    DFLY_ASSERT(queue->comp_ch);
    //TODO: Check and fail on error

    cqc = queue->recv_qd + 2 * queue->send_qd;
    //TODO: Verify this calculation is valid for this queue

    cq = ibv_create_cq(id->verbs, cqc, NULL, queue->comp_ch,
                    queue->host_qid % id->verbs->num_comp_vectors);
    DFLY_ASSERT(cq);

    queue->cq = cq;
    queue->ncqe = cqc;

    //TODO: Check and fail on error
    ibv_req_notify_cq(cq, 0);

    asprintf(&rdd_conn_id, "rdd_%s", queue->l->listen_ip);
    DFLY_ASSERT(rdd_conn_id != NULL);
    icore = dfly_get_next_core(rdd_conn_id, g_dragonfly->rddcfg->num_cq_cores_per_ip , queue->l->listen_ip, NULL);
    spdk_cpuset_zero(&cset);
    spdk_cpuset_set_cpu(&cset, icore, true);

    DFLY_ASSERT(queue->th.spdk.cq_thread == NULL);
    snprintf(thread_name, MAX_LISTENER_NAME, "rdd_qp_%p", queue);
    //Created thread in the current core
    //TODO: Check if same thread can be used to run multiple pollers
    queue->th.spdk.cq_thread = spdk_thread_create(thread_name, &cset);
    if(!queue->th.spdk.cq_thread) {
        DFLY_ERRLOG("Failed to create cq poll thread %s\n", thread_name);
        DFLY_ASSERT(0);//To do call destroy function and handle
        //rdd_destroy(ctx);
        return -1;
    }

    spdk_thread_send_msg(queue->th.spdk.cq_thread, _rdd_start_cq_poller, (void *)queue);
    //TODO: Wait for initialization

    memset(&qp_attr, 0, sizeof(struct ibv_qp_init_attr));

    qp_attr.send_cq = cq;
    qp_attr.recv_cq = cq;
    qp_attr.qp_type = IBV_QPT_RC;

    qp_attr.cap.max_send_wr = 2 * queue->send_qd;
    qp_attr.cap.max_recv_wr = queue->recv_qd;
    qp_attr.cap.max_send_sge = queue->ctx->max_sgl_segs;
    qp_attr.cap.max_recv_sge = queue->ctx->max_sgl_segs;

    rc = rdma_create_qp(id, queue->pd, &qp_attr);
    if(rc == -1) {
        DFLY_ERRLOG("rdma_create_qp failed with errno %d\n", errno);
        return rc;
    }

    queue->qp = id->qp;

    for(i=0; i<queue->send_qd; i++) {
        struct rdd_rsp_s *rsp = &queue->rsps[i];
        
        rc = ibv_post_recv(queue->qp, &rsp->recv_wr, NULL);
        DFLY_ASSERT(rc == 0);
        //TODO: Handle error conditions
    }

    return rc;
}

static int rdd_queue_alloc_reqs(struct rdd_rdma_queue_s *queue)
{
    int i;

    queue->reqs = (struct rdd_req_s *)calloc(queue->send_qd, sizeof(struct rdd_req_s));
    queue->cmds = (rdd_rdma_cmd_t *)calloc(queue->send_qd, sizeof(rdd_rdma_cmd_t));
    queue->rsps = (struct rdd_rsp_s *)calloc(queue->send_qd, sizeof(struct rdd_rsp_s));
    queue->data_sges = (struct ibv_sge *)calloc(queue->send_qd, sizeof(struct ibv_sge) * queue->ctx->max_sgl_segs);

    if(!queue->reqs || !queue->cmds || !queue->rsps || !queue->data_sges) {

        if(queue->reqs) free(queue->reqs);
        if(queue->cmds) free(queue->cmds);
        if(queue->rsps) free(queue->rsps);
        if(queue->data_sges) free(queue->data_sges);

        DFLY_ERRLOG("Failed to alloc requests for queue %p\n", queue);
        //rdd_queue_destroy(queue);//TODO: ON return of error code
        return -1;
    }

    queue->cmd_mr = ibv_reg_mr(queue->pd, queue->cmds,
                        queue->send_qd * sizeof(rdd_rdma_cmd_t), 
         					IBV_ACCESS_LOCAL_WRITE |
						    IBV_ACCESS_REMOTE_WRITE |
							IBV_ACCESS_REMOTE_READ);                   

    queue->rsp_mr = ibv_reg_mr(queue->pd, queue->rsps,
                        queue->send_qd * sizeof(struct rdd_rsp_s), 
         					IBV_ACCESS_LOCAL_WRITE |
						    IBV_ACCESS_REMOTE_WRITE |
							IBV_ACCESS_REMOTE_READ);          

    if(!queue->cmd_mr || !queue->rsp_mr) {
        DFLY_ERRLOG("Failed to register memory regions for queue %p error %d\n", queue, errno);
        //rdd_queue_destroy(queue);//TODO: ON return of error code
        return -1;
    }         

    for(i=0; i< queue->send_qd; i++) {
        queue->reqs[i].rdd_cmd = &queue->cmds[i];

        queue->reqs[i].rdd_cmd->cid = i;

        queue->reqs[i].req.send_sge.addr = (uint64_t)queue->reqs[i].rdd_cmd;
        queue->reqs[i].req.send_sge.length = sizeof(rdd_rdma_cmd_t);
        queue->reqs[i].req.send_sge.lkey = queue->cmd_mr->lkey;

        queue->reqs[i].req.send_wr.opcode = IBV_WR_SEND;
        queue->reqs[i].req.rdd_wr.type    = RDD_WR_TYPE_REQ_SEND;
        queue->reqs[i].req.send_wr.wr_id = (uint64_t)&queue->reqs[i].req.rdd_wr;
        queue->reqs[i].req.send_wr.sg_list = &queue->reqs[i].req.send_sge;
        queue->reqs[i].req.send_wr.num_sge = 1;
        queue->reqs[i].req.send_wr.next = NULL;
        queue->reqs[i].req.send_wr.send_flags = IBV_SEND_SIGNALED;
        queue->reqs[i].req.send_wr.imm_data = 0;
        queue->reqs[i].id = i;
        queue->reqs[i].rsp_idx = -1;

        queue->reqs[i].data.data_sge = queue->data_sges + (queue->ctx->max_sgl_segs * i);
        queue->reqs[i].data.data_wr.wr_id = (uint64_t)&queue->reqs[i].data.rdd_wr;
        queue->reqs[i].data.data_wr.sg_list = queue->reqs[i].data.data_sge;
        queue->reqs[i].data.data_wr.num_sge = 1;
        queue->reqs[i].data.data_wr.next = NULL;
        queue->reqs[i].data.data_wr.send_flags = IBV_SEND_SIGNALED;
        queue->reqs[i].data.data_wr.imm_data = 0;

        queue->rsps[i].rdd_wr.type   = RDD_WR_TYPE_RSP_RECV;
        queue->rsps[i].recv_sge.addr = (uint64_t)&queue->rsps[i].rsp;
        queue->rsps[i].recv_sge.length = sizeof(rdd_rdma_rsp_t);
        queue->rsps[i].recv_sge.lkey = queue->rsp_mr->lkey;
        queue->rsps[i].idx = i;

        queue->rsps[i].recv_wr.wr_id = (uint64_t)&queue->rsps[i].rdd_wr;
        queue->rsps[i].recv_wr.sg_list = &queue->rsps[i].recv_sge;
        queue->rsps[i].recv_wr.num_sge = 1;
        queue->rsps[i].recv_wr.next = NULL;
        queue->rsps[i].q = queue;

        queue->reqs[i].state = RDD_REQ_FREE;
        queue->reqs[i].q = queue;

        TAILQ_INSERT_TAIL(&queue->free_reqs, &queue->reqs[i], link);
    }

    return 0;
}

int rdd_queue_accept(struct rdd_rdma_listener_s  *l, struct rdma_cm_event *ev)
{
    struct rdma_cm_id *id = ev->id;
    struct rdma_conn_param conn_param = {};
    struct rdma_conn_param *cl_conn_param = &ev->param.conn;
    struct ibv_device_attr cl_dev_attr;

    struct rdd_queue_priv_s priv = {};

    int ret;
    struct rdd_rdma_queue_s *queue = (struct rdd_rdma_queue_s *)id->context;

    ret = ibv_query_device(id->verbs, &cl_dev_attr);
    if(ret) {
        DFLY_ERRLOG("Failed to query device attribute %s\n", id->verbs->device->name);
        return -1;
    }

	queue->qhandle = rdd_reg_queue2ctx(l->prctx, queue);
	if(queue->qhandle == RDD_INVALID_CHANDLE(l->prctx)) {
		DFLY_ERRLOG("Qhandle generate failed for ctx %p\n", l->prctx);
		return -1;
	}

    priv.data.server.proto_ver = RDD_PROTOCOL_VERSION;
    priv.data.server.qhandle = queue->qhandle;

    conn_param.rnr_retry_count = 7;
    conn_param.flow_control = 1;
    if((uint8_t)cl_dev_attr.max_qp_init_rd_atom < cl_conn_param->initiator_depth) {
        conn_param.initiator_depth = (uint8_t)cl_dev_attr.max_qp_init_rd_atom;
    } else {
        conn_param.initiator_depth = cl_conn_param->initiator_depth;
    }
    conn_param.responder_resources = conn_param.initiator_depth;
    conn_param.private_data = &priv;
    conn_param.private_data_len = sizeof(priv);

    ret = rdma_accept(id, &conn_param);
    if(ret) {
        DFLY_ERRLOG("rdma_accept failed error code (%d) \n", ret);
        return -1;
    }

    return 0;
}

int rdd_queue_connect(struct rdd_rdma_listener_s  *l, struct rdma_cm_event *ev)
{
    struct rdma_cm_id *id = ev->id;
    struct rdd_queue_priv_s *priv = (struct rdd_queue_priv_s *)ev->param.conn.private_data;
    //TODO: Check if same thread can be used to run multiple pollers
    struct sockaddr *peer_saddr;
    struct sockaddr_in *s4addr_in;
	struct sockaddr_in6 *s6addr_in;

    struct rdd_rdma_queue_s *queue = NULL;

    int rc = 0;

    //DFLY_ASSERT(ctx->tr.rdma.cm_id == id);//TODO: This could be differnt
    DFLY_ASSERT(spdk_get_thread() == l->th.spdk.cm_thread);

    if(priv->data.client.proto_ver != RDD_PROTOCOL_VERSION) {
        DFLY_ERRLOG("Unsupported client version %x\n", priv->data.client.proto_ver);
        return -1;
    }

    queue = (struct rdd_rdma_queue_s *)calloc(1, sizeof(struct rdd_rdma_queue_s));
    if(!queue) {
        DFLY_ERRLOG("Failed to allocate queue\n");
        return -1;
    }

    queue->ctx = l->prctx;//TODO: Check if this needs to be listener
    queue->cm_id = id;
    queue->host_qid = priv->data.client.qid;
    queue->recv_qd = priv->data.client.hsqsize;
    queue->send_qd = priv->data.client.hrqsize;
	queue->outstanding_qd = 0;
    queue->state = RDD_QUEUE_CONNECTING;
    queue->l = l;

    peer_saddr = rdma_get_peer_addr(id);
    switch(peer_saddr->sa_family) {
        case AF_INET:
            s4addr_in = (struct sockaddr_in *)peer_saddr;
            inet_ntop(AF_INET, &s4addr_in->sin_addr,
                queue->peer_ip, INET6_ADDRSTRLEN);
            break;
        case AF_INET6:
            s6addr_in = (struct sockaddr_in6 *)peer_saddr;
            inet_ntop(AF_INET6, &s6addr_in->sin6_addr,
                queue->peer_ip, INET6_ADDRSTRLEN);
            break;
        default:
            DFLY_NOTICELOG("Unknown peer IP address family\n");
    }

    pthread_mutex_init(&queue->init_lock, NULL);

	queue->submit_latency = 0;
	queue->submit_count = 0;

    TAILQ_INIT(&queue->free_reqs);
    TAILQ_INIT(&queue->outstanding_reqs);
    TAILQ_INIT(&queue->pending_reqs);//DSS Request queue

    queue->pd = l->device->pd;
    queue->map = l->device->map;

    id->context = queue;//Got back on connection establishment from client

    rc = rdd_queue_alloc_reqs(queue);
    if(rc) {
        rdd_queue_destroy(queue);
        return -1;
    }

    rc = rdd_queue_ib_create(queue);
    if(rc) {
        rdd_queue_destroy(queue);
        return -1;
    }

    rc = rdd_queue_accept(l, ev);
    DFLY_ASSERT(rc == 0);
    
    return 0;
}

int rdd_queue_destroy(struct rdd_rdma_queue_s *queue)
{
    int rc = 0;

    //TODO: Dealloc ibv queues
    //DFLY_ASSERT(0);

    //Dealloc qhandle
    //
    //Revert post recvs
    //
    //revert rdma_create_qp
    //
    //stop cq poller
    //
    //stop cq thread
    //
    //dealloc dfly numa core
    //
    //revert ibv_req_notify_cq??
    //
    //revert ibv_create_cq
    //
    //revert ivb_create_comp_channel
    //
    //ibv_dereg MR (cmd_mr, rsp_mr_
    //
    //mutex destroy??

    if(queue->cmds) {
        free(queue->cmds);
        queue->cmds = NULL;
    }

    if(queue->rsps) {
        free(queue->rsps);
        queue->rsps = NULL;
    }

    if(queue->reqs) {
        free(queue->reqs);
        queue->reqs = NULL;
    }

    if(queue->data_sges) {
        free(queue->data_sges);
        queue->data_sges = NULL;
    }

    free(queue);

    return rc;
}

int rdd_cm_event_handler(struct rdd_rdma_listener_s  *l, struct rdma_cm_event *ev)
{
	int r = 0;

    //DFLY_NOTICELOG("Processing event %s\n", rdma_event_str(ev->event));
	switch (ev->event) {
        case RDMA_CM_EVENT_CONNECT_REQUEST:
            r = rdd_queue_connect(l, ev);
            //Connect queue
            break;
        case RDMA_CM_EVENT_ESTABLISHED:
            r = rdd_cl_queue_established(l, ev);
            break;
        case RDMA_CM_EVENT_DISCONNECTED:
			//TODO : destroy queues 
			//			deregister qhandles
		    //DFLY_ERRLOG("Disconnect event %p for ctx %p\n", ev, ctx);
			rdd_cl_queue_disconnect(l, ev);
            break;
	    default:
		    DFLY_ERRLOG("Unhandled event %d for %p\n", ev->event, l);
	}
	return r;    
}

int rdd_cm_event_task(void *arg)
{
    struct rdd_rdma_listener_s  *l = (struct rdd_rdma_listener_s  *)arg;

	struct rdma_event_channel *ch = l->tr.rdma.ev_channel;
	struct rdma_cm_event *event;
    int rc;

    rc = poll(&l->th.spdk.cm_poll_fd, 1, 0);
    if (rc == 0) {
        return SPDK_POLLER_IDLE;
    }

    if(rc < 0) {
        //TODO: Handle Error
        DFLY_ERRLOG("cm event poll failed with error [%d]: %s\n", errno, spdk_strerror(errno));
    }

    while(1) {
	    rc = rdma_get_cm_event(ch, &event);
        if (rc) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
                DFLY_ERRLOG("Rdma event retrieval failed for %p with error [%d]: [%s]\n", l, errno, spdk_strerror(errno));
			}
			break;
		}
        if(rdd_cm_event_handler(l, event)) {
            DFLY_ERRLOG("CM event handler failed for %p event %p\n", l, event);
                //TODO: Connect Error stop poller and exit
        }
		rdma_ack_cm_event(event);
  	} 
	return SPDK_POLLER_BUSY;
}

static void _rdd_start_cm_event_poller(void *arg)
{
    struct rdd_rdma_listener_s  *l = (struct rdd_rdma_listener_s  *)arg;

    int flags, rc;

    flags = fcntl(l->tr.rdma.ev_channel->fd, F_GETFL);
    rc = fcntl(l->tr.rdma.ev_channel->fd, F_SETFL, flags | O_NONBLOCK);
    if (rc < 0) {
        DFLY_ERRLOG("Failed to change file descriptor of cm event channel\n");
        DFLY_ASSERT(rc == 0);;
        return; //Handle error scenario
    }

    l->th.spdk.cm_poll_fd.fd = l->tr.rdma.ev_channel->fd;
    l->th.spdk.cm_poll_fd.events = POLLIN;
    l->th.spdk.cm_poll_fd.revents = 0;

    DFLY_ASSERT(spdk_get_thread() == l->th.spdk.cm_thread);
    DFLY_ASSERT(l->th.spdk.cm_poller == NULL);
    l->th.spdk.cm_poller = SPDK_POLLER_REGISTER(rdd_cm_event_task, l, RDD_DEFAULT_CM_POLL_PERIOD_IN_US);
    DFLY_ASSERT(l->th.spdk.cm_poller != NULL);

    DFLY_NOTICELOG("Started cm event poll thread for %p on core %d \n", l, spdk_env_get_current_core());
}

static void _rdd_stop_cm_event_poller(void *arg)
{
    struct rdd_rdma_listener_s  *l = (struct rdd_rdma_listener_s  *)arg;

    DFLY_ASSERT(l->th.spdk.cm_poller != NULL);
    DFLY_ASSERT(l->th.spdk.cm_thread != NULL);
    DFLY_ASSERT(spdk_get_thread() == l->th.spdk.cm_thread);

    spdk_poller_unregister(&l->th.spdk.cm_poller);
    l->th.spdk.cm_poller = NULL;
    spdk_thread_exit(l->th.spdk.cm_thread);
    l->th.spdk.cm_thread = NULL;

    return;
}

void _rdd_listener_init(void *arg, void *dummy);

int rdd_listener_add(rdd_ctx_t *ctx, char *ip, char *port)
{
	int rc = 0;
    struct addrinfo hints;

	struct rdd_rdma_listener_s *l;
    struct rdd_rdma_device_s *device;

    memset(&hints, 0, sizeof(hints));
    //hints.ai_family = AF_UNSPEC;//Try to resolve IPV4 or IPV6 address
    //hints.ai_family = AF_INET;
    hints.ai_family = AF_UNSPEC;//Try to resolve IPV4 or IPV6 address
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

	l = calloc(1, sizeof(struct rdd_rdma_listener_s));
	if(!l) {
		return -1;
	}

	//TODO: Alloc listener and change variable names
    l->listen_ip = strdup(ip);
    l->listen_port = strdup(port);

    TAILQ_INIT(&l->queues);

	l->prctx = ctx;

	rc = getaddrinfo(ip, port, &hints, &l->ai);
    if(rc) {
        DFLY_ERRLOG("Failed to get addrinfo ip %s port %s error '%s' (%d)\n", 
                        ip, port, gai_strerror(rc), rc);
        goto err;
    }

    l->tr.rdma.ev_channel = rdma_create_event_channel();
    if(!l->tr.rdma.ev_channel) {
        DFLY_ERRLOG("rdma_create_event_channel failed for ip %s port %s %d\n",
                        ip, port, errno);
        goto err;
    }

    rc = rdma_create_id(l->tr.rdma.ev_channel, &l->tr.rdma.cm_id, l, RDMA_PS_TCP);
    if (rc) {
        //rc == -1
        DFLY_ERRLOG("rdma_create_id failed for ip %s port %s %d\n",
                ip, port, errno);
        goto err;
    }

    //TODO: bind/listen to all available addr in ai
    //Now binding only to the first
    rc = rdma_bind_addr(l->tr.rdma.cm_id, l->ai->ai_addr);
    if(rc) {
        //rc == -1
        DFLY_ERRLOG("rdma_bind_addr failed for ip %s port %s %d\n",
                ip, port, errno);
        goto err;
    }

    rc = rdma_listen(l->tr.rdma.cm_id, RDD_DEFAULT_LISTEN_BACKLOG);
    if(rc) {
        //rc == -1
        DFLY_ERRLOG("rdma_listen failed for ip %s port %s %d\n",
                ip, port, errno);
        goto err;
    }


	TAILQ_FOREACH(device, &ctx->devices, link) {
		if (device->context == l->tr.rdma.cm_id->verbs) {
			l->device = device;
			break;
		}
	}
	if (!l->device) {
		DFLY_ERRLOG("Accepted a connection with verbs %p, but unable to find a corresponding device.\n",
			    l->tr.rdma.cm_id->verbs);
        goto err;
	}

    TAILQ_INSERT_TAIL(&ctx->listeners, l, link);
	//TODO : Track Listener context live/destroying

	_rdd_listener_init(l, NULL);

	return 0;

err:
	if(l->ai) {
		free(l->ai);
	}

    if(l->tr.rdma.cm_id) {
        rc = rdma_destroy_id(l->tr.rdma.cm_id);
        if(!rc) {
            //rc == -1
            //ALL QP need to be free and    //all events acked before destroy
            DFLY_ERRLOG("rdma_destroy_id failed for ip %s port %s %d\n",
                l->listen_ip, l->listen_port, errno);
        }
    }

    if(l->tr.rdma.ev_channel) {
        rdma_destroy_event_channel(l->tr.rdma.ev_channel);
	}

	if(l)
		free(l);

	return rc;
}

//Call this function on the core where the cm_event thread needs to run
rdd_ctx_t *rdd_init(rdd_cfg_t *c, rdd_params_t params)
{
    rdd_ctx_t *ctx;

    int i, rc;

    struct ibv_context **contexts;
    struct rdd_rdma_device_s *device;

    ctx = (rdd_ctx_t *)calloc(1, sizeof(rdd_ctx_t));
    if(!ctx) {//Context alloc failed
        DFLY_ERRLOG("context alloc failed\n");
        return NULL;
    }

	TAILQ_INIT(&ctx->devices);
	TAILQ_INIT(&ctx->listeners);

	rc = pthread_rwlock_init(&ctx->handle_ctx.rwlock, NULL);
	DFLY_ASSERT(rc == 0);//TODO: Process error code

	ctx->handle_ctx.nuse = 0;
	ctx->handle_ctx.nhandles = RDD_DEFAULT_MIN_CHANDLE_COUNT;
	ctx->handle_ctx.handle_arr = calloc(RDD_DEFAULT_MIN_CHANDLE_COUNT, sizeof(ctx->handle_ctx.handle_arr));
	DFLY_NOTICELOG("ctx array element size %d\n", sizeof(ctx->handle_ctx.handle_arr));

	ctx->max_sgl_segs = params.max_sgl_segs;
    ctx->num_cq_cores_per_ip = c->num_cq_cores_per_ip;
	DFLY_DEBUGLOG(DSS_RDD, "%d max sgl segments configured\n", ctx->max_sgl_segs);
	DFLY_DEBUGLOG(DSS_RDD, "%d cq cores alocated per rdd listen\n", ctx->max_sgl_segs);

	srand(time(NULL));
	ctx->handle_ctx.hmask = (uint16_t)rand();
	DFLY_NOTICELOG("Generated handle mask %x\n", ctx->handle_ctx.hmask);

    contexts = rdma_get_devices(NULL);
    if(contexts == NULL) {
        DFLY_ERRLOG("rdma_get_devices() failed: %s (%d)\n", spdk_strerror(errno), errno);
        goto err;
    }

    i = 0;
    while(contexts[i] != NULL) {
        device = calloc(1, sizeof(*device));
        if(!device) {
            DFLY_ERRLOG("Unable to allocate memory for rdma device\n");
            goto err;
        }

        device->context = contexts[i];
        device->pd = ibv_alloc_pd(device->context);
        if(!device->pd) {
            DFLY_ERRLOG("Failed to allocate protection domain for device\n");
            goto err;
        }

        device->map = spdk_mem_map_alloc(0, &g_rdd_rdma_map_ops, device->pd);
        if(!device->map) {
            DFLY_ERRLOG("Failed to allocate memory map for device\n");
            goto err;
        }
        TAILQ_INSERT_TAIL(&ctx->devices, device, link);
        i++;
    }
    rdma_free_devices(contexts);

	for(i=0; i < c->n_ip; i++) {
		rc = rdd_listener_add(ctx, c->conn_info[i].ip, c->conn_info[i].port);
		if(rc != 0) {
			DFLY_ERRLOG("Error Listening on $s ip %s port\n", \
							c->conn_info[i].ip, \
							c->conn_info[i].port);
			rdd_destroy(ctx);
			return NULL;
		}
	}

	return ctx;

err:
    rdd_destroy(ctx);
    return NULL;
}

void _rdd_listener_init(void *arg, void *dummy)
{
	uint32_t icore;
	struct spdk_event *event;

    struct rdd_rdma_listener_s  *listener;
    char thread_name[MAX_LISTENER_NAME] = {0};

	listener = (struct rdd_rdma_listener_s *)arg;

	icore = dfly_get_next_core("RDD_CQ_POLLER", 1, NULL, NULL);

	if (spdk_env_get_current_core() != icore) {
		event = spdk_event_allocate(icore, _rdd_listener_init, arg, NULL);
		assert(event != NULL);
		spdk_event_call(event);

		return;
	}

    DFLY_NOTICELOG("Starting cm thread on %u core\n", icore);

    snprintf(thread_name, MAX_LISTENER_NAME, "rdd_%p", listener->tr.rdma.cm_id);
    //Created thread in the current core
    listener->th.spdk.cm_thread = spdk_thread_create(thread_name, NULL);
    if(!listener->th.spdk.cm_thread) {
        DFLY_ERRLOG("Failed to create cm event thread %s\n", thread_name);
        rdd_destroy(listener->prctx);
        DFLY_ASSERT(listener->th.spdk.cm_thread != NULL);
        return;
    }

    spdk_thread_send_msg(listener->th.spdk.cm_thread, _rdd_start_cm_event_poller, (void *)listener);
    //TODO: Wait for initialization

    DFLY_NOTICELOG("Listening on IP %s port %s\n", listener->listen_ip, listener->listen_port);
    return;
}

void rdd_stop_listener(struct rdd_rdma_listener_s  *l)
{
    int rc;

    DFLY_NOTICELOG("Stop Listening on IP %s port %s\n", l->listen_ip, l->listen_port);

    if(l->th.spdk.cm_thread) {
        spdk_thread_send_msg(l->th.spdk.cm_thread, _rdd_stop_cm_event_poller, (void *) l);
        //TODO: Synchronize Stop thread and poller
    }

    if(l->tr.rdma.ev_channel) {
        rdma_destroy_event_channel(l->tr.rdma.ev_channel);
    }

    if(l->tr.rdma.cm_id) {
        rc = rdma_destroy_id(l->tr.rdma.cm_id);
        if(!rc) {
            //rc == -1
            //ALL QP need to be free and all events acked before destroy
            DFLY_ERRLOG("rdma_destroy_id failed for ip %s port %s %d\n",
                l->listen_ip, l->listen_port, errno);
        }
    }
    
    if(l->ai) {
        freeaddrinfo(l->ai);
    }

    if(l->listen_ip) {
        free(l->listen_ip);
    }

    if(l->listen_port) {
        free(l->listen_port);
    }

	free(l);

	return;
}

void rdd_destroy(rdd_ctx_t *ctx)
{
	struct rdd_rdma_listener_s *l, *tl;

	TAILQ_FOREACH_SAFE(l, &ctx->listeners, link, tl) {
		TAILQ_REMOVE(&ctx->listeners, l, link);
		rdd_stop_listener(l);
	}
	//TODO: Make sure all queues exited
	pthread_rwlock_destroy(&ctx->handle_ctx.rwlock);

    if(ctx) {
        free(ctx);
    }
}
