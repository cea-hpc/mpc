#include "lcp_rndv.h"

#include "lcp_ep.h"
#include "lcp_prototypes.h"
#include "lcp_mem.h"
#include "lcp_context.h"
#include "lcp_task.h"
#include "lcp_datatype.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

//FIXME: function also needed in lcp_mem.c
/**
 * @brief Build a memory bitmap.
 * 
 * @param length length of bitmap
 * @param min_frag_size minimum fragment size
 * @param max_iface maximum number of interfaces
 * @param start_idx starting index
 * @param bmap_p output bitmap
 */
static inline void build_memory_bitmap(size_t length, 
                                       size_t min_frag_size, 
                                       int max_iface,
                                       int start_idx,
                                       bmap_t *bmap_p)
{
        bmap_t bmap = MPC_BITMAP_INIT;
        int num_used_ifaces = 0;

        while ((length > num_used_ifaces * min_frag_size) &&
               (num_used_ifaces < max_iface)) {
                MPC_BITMAP_SET(bmap, start_idx);
                start_idx = (start_idx + 1) % max_iface;
                num_used_ifaces++;
        }

        *bmap_p = bmap;
}

/* ============================================== */
/* Packing                                        */
/* ============================================== */

/**
 * @brief Pack data for rget.
 * 
 * @param dest destination header
 * @param data request to pack
 * @return size_t size of packed data
 */
static size_t lcp_rts_rget_pack(void *dest, void *data)
{
        lcp_rndv_hdr_t *hdr = dest;
        lcp_request_t *req = data;
        void *rkey_buf;
        int packed_size;

        hdr->base.comm     = req->send.tag.comm;
        hdr->base.src_tid  = req->send.tag.src_tid;
        hdr->base.dest_tid = req->send.tag.dest_tid;
        hdr->base.tag      = req->send.tag.tag;
        hdr->base.seqn     = req->seqn;

        hdr->src_pid   = req->send.tag.src_pid;
        hdr->msg_id    = req->msg_id;
        hdr->size      = req->send.length;

        rkey_buf = hdr + 1;
        packed_size = lcp_mem_rkey_pack(req->ctx, req->state.lmem,
                                        rkey_buf);
        return sizeof(*hdr) + packed_size;
}

/**
 * @brief Pack data for rput
 * 
 * @param dest destination header
 * @param data request to pack
 * @return size_t size of packed data
 */
static size_t lcp_rts_rput_pack(void *dest, void *data)
{
        lcp_rndv_hdr_t *hdr = dest;
        lcp_request_t *req = data;

        hdr->base.comm     = req->send.tag.comm;
        hdr->base.src_tid  = req->send.tag.src_tid;
        hdr->base.dest_tid = req->send.tag.dest_tid;
        hdr->base.tag      = req->send.tag.tag;
        hdr->base.seqn     = req->seqn;

        hdr->src_pid   = req->send.tag.src_pid;
        hdr->msg_id    = req->msg_id;
        hdr->size      = req->send.length;

        return sizeof(*hdr);
}

/**
 * @brief Pacl data for rendez-vous finalization message
 * 
 * @param dest destination header
 * @param data request to pack
 * @return size_t size of packed data
 */
static size_t lcp_rndv_fin_pack(void *dest, void *data)
{
        lcp_ack_hdr_t *hdr = dest;
        lcp_request_t *req = data;

        hdr->msg_id = req->msg_id;

        return sizeof(*hdr);
}

/**
 * @brief Pack data for rput.
 * 
 * @param dest destination header
 * @param data request to pack
 * @return size_t size of packed data
 */
static size_t lcp_rtr_rput_pack(void *dest, void *data)
{
        lcp_ack_hdr_t *hdr = dest;
        lcp_request_t *req = data;
        lcp_request_t *super = req->super;
        int packed_size;
        void *rkey_ptr;

        hdr->msg_id = super->msg_id;

        rkey_ptr = hdr + 1;
        packed_size = lcp_mem_rkey_pack(super->ctx, super->state.lmem, 
                                        rkey_ptr);

        return sizeof(*hdr) + packed_size;
}

/* ============================================== */
/* Rendez-vous send completion callback           */
/* ============================================== */

/**
 * @brief Send a rput request.
 * 
 * @param super request to send
 * @param rmem // TODO ?
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_rput_common(lcp_request_t *super, lcp_mem_h rmem)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        size_t remaining, offset;
        size_t frag_length, length;
        lcp_ep_h ep;
        lcr_rail_attr_t attr;
        _mpc_lowcomm_endpoint_t *lcr_ep;

        /* Get source address */
        void *start = super->datatype & LCP_DATATYPE_CONTIGUOUS ?
                super->send.buffer : super->state.pack_buf;

        remaining   = super->send.length;
        offset      = 0;
        ep          = super->send.ep;

        while (remaining > 0) {
                super->state.cc = lcp_ep_get_next_cc(ep);
                if (!MPC_BITMAP_GET(rmem->bm, super->state.cc)) {
                        continue;
                }
                lcr_ep = ep->lct_eps[super->state.cc];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                /* length is min(max_frag_size, per_ep_length) */
                frag_length = attr.iface.cap.rndv.max_get_zcopy;
                length = remaining < frag_length ? remaining : frag_length;

                //FIXME: error managment => NO_RESOURCE not handled
                rc = lcp_send_do_put_zcopy(lcr_ep,
                                           (uint64_t)start + offset,
                                           offset,
                                           &(rmem->mems[super->state.cc]),
                                           length,
                                           &(super->state.comp));

                offset += length; remaining -= length;
        }

        return rc;
}

/**
 * @brief Complete a rput active message
 * 
 * @param comp completion
 */
void lcp_request_complete_am_rput(lcr_completion_t *comp)
{
        int payload;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              state.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_ep_h ep = req->send.ep;

                payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                               MPC_LOWCOMM_RFIN_MESSAGE,
                                               lcp_rndv_fin_pack,
                                               req);
                if (payload < 0) {
                        mpc_common_debug_error("LCP: error sending fin message");
                }

                lcp_request_complete(req);
        }
}


/* ============================================== */
/* Protocol starts                                */
/* ============================================== */

/**
 * @brief Start an rget protocol with active message
 * 
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS In case of success
 */
int lcp_send_am_rget_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int payload_size;
        lcr_rail_attr_t attr;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = req->state.cc;
        sctk_rail_info_t *iface = ep->lct_eps[cc]->rail;

        mpc_common_debug_info("LCP: send rndv get am req=%p, comm_id=%d, tag=%d, "
                              "src=%d, dest=%d, msg_id=%d, buf=%p.", req, 
                              req->send.tag.comm, 
                              req->send.tag.tag, req->send.tag.src_tid, 
                              req->send.tag.dest_tid, req->msg_id,
                              req->send.buffer);

        /* memory registration */
        lcp_mem_create(req->ctx, &(req->state.lmem));

        /* Get source address */
        void *start = req->datatype & LCP_DATATYPE_CONTIGUOUS ?
                req->send.buffer : req->state.pack_buf;
        /* Register and pack memory pin context that will be sent to remote */
        //FIXME: having the bitmap in the request state seems not useful. Could
        //       be only in the lcp_mem_h
        req->state.lmem->bm = ep->conn_map;
        rc = lcp_mem_reg_from_map(req->send.ep->ctx, 
                                  req->state.lmem,
                                  req->state.lmem->bm,
                                  start, req->send.length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        payload_size = lcp_send_do_am_bcopy(ep->lct_eps[cc], 
                                            MPC_LOWCOMM_RGET_MESSAGE,
                                            lcp_rts_rget_pack, req);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
        }

        /* Do not complete request right now */
        //NOTE: no completion function for am rget since request is completed
        //      with fin handler.
err:
        return rc;
}

/**
 * @brief Start a rput protocol with active messages
 * 
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_am_rput_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = req->state.cc;
        sctk_rail_info_t *iface = ep->lct_eps[cc]->rail;

        mpc_common_debug_info("LCP: send rndv put am req=%p, comm_id=%d, tag=%d, "
                              "src=%d, dest=%d, msg_id=%d, buf=%p.", req, 
                              req->send.tag.comm, 
                              req->send.tag.tag, req->send.tag.src_tid, 
                              req->send.tag.dest_tid, req->msg_id,
                              req->send.buffer);

        /* Register and pack memory pin context that will be sent to remote */
        //NOTE: memory registration for put not necessary for portals.
        //      however, needed probably for other rma transports.
        //      Sender is responsible to choose which communication channel will
        //      be used. So the memory bitmap is packed and sent.
        /* memory registration */
        lcp_mem_create(req->ctx, &(req->state.lmem));

        /* Get source address */
        void *start = req->datatype & LCP_DATATYPE_CONTIGUOUS ?
                req->send.buffer : req->state.pack_buf;

        /* Register and pack memory pin context that will be sent to remote */
        req->state.lmem->bm = ep->conn_map;
        rc = lcp_mem_reg_from_map(req->send.ep->ctx, 
                                  req->state.lmem,
                                  req->state.lmem->bm,
                                  start, 
                                  req->send.length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_request_complete_am_rput
        };

        payload_size = lcp_send_do_am_bcopy(ep->lct_eps[cc], 
                                            MPC_LOWCOMM_RPUT_MESSAGE,
                                            lcp_rts_rput_pack, req);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
        }

        /* Do not complete request right now */
err:
        return rc;
}

/**
 * @brief Start a rendez-vous depending on rendez-vous mode
 * 
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_rndv_am_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep = req->send.ep;

        if (lcp_pending_create(req->ctx->pend, req, req->msg_id) == NULL) {
                mpc_common_debug_error("LCP: could not add pending message");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        /* delete from pending list on completion */
        req->flags |= LCP_REQUEST_DELETE_FROM_PENDING; 


        //FIXME: below piece of code can be factorized with
        //       lcp_send_rndv_offload_start 
        /* Buffer must be allocated and data packed to make it contiguous
         * and use zcopy and memory registration. */
        if (req->datatype & LCP_DATATYPE_DERIVED) {
                req->state.pack_buf = sctk_malloc(req->send.length);
                if (req->state.pack_buf == NULL) {
                        mpc_common_debug_error("LCP: could not allocate pack "
                                               "buffer");
                        return MPC_LOWCOMM_ERROR;
                }

                lcp_datatype_pack(req->ctx, req, req->datatype,
                                  req->state.pack_buf, NULL, 
                                  req->send.length);
        }

        switch (ep->ctx->config.rndv_mode) {
        case LCP_RNDV_GET:
                req->send.func = lcp_send_am_rget_start;
                break;
        case LCP_RNDV_PUT:
                req->send.func = lcp_send_am_rput_start;
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown protocol");
                break;
        }

err:
        return rc;
}

void lcp_request_complete_am_rget(lcr_completion_t *comp);
int lcp_recv_am_rput(lcp_request_t *req);
int lcp_recv_am_rget(lcp_request_t *req);

/**
 * @brief Respond to a rendez-vous start message
 * 
 * @param rreq return request
 * @param hdr rendez-vous header
 * @param length length of header
 * @param rndv_mode rendez-vous mode (rget, rput)
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_rndv_matched(lcp_request_t *rreq,
                     lcp_rndv_hdr_t *hdr,
                     size_t length,
                     lcp_rndv_mode_t rndv_mode) 
{
        int rc; 

        rreq->recv.tag.src_pid  = hdr->src_pid;
        rreq->recv.tag.src_tid  = hdr->base.src_tid;
        rreq->recv.tag.tag      = hdr->base.tag;
        //TODO: on receiver msg_id might not be unique, it has to be completed
        //      with sender unique MPI identifier.
        rreq->msg_id            = hdr->msg_id;
        rreq->recv.send_length  = hdr->size; 
        rreq->state.remaining   = hdr->size;
        rreq->state.offset      = 0;
        rreq->seqn              = hdr->base.seqn;

        //FIXME: below piece of code can be factorized with
        //       lcp_send_rndv_offload_start 
        /* Buffer must be allocated and data packed to make it contiguous
         * and use zcopy and memory registration. */
        if (rreq->datatype & LCP_DATATYPE_DERIVED) {
                rreq->state.pack_buf = sctk_malloc(rreq->recv.send_length);
                if (rreq->state.pack_buf == NULL) {
                        mpc_common_debug_error("LCP: could not allocate pack "
                                               "buffer");
                        return MPC_LOWCOMM_ERROR;
                }
        }

        switch (rndv_mode) {
        case LCP_RNDV_PUT:
                /* For RPUT, data is unpacked upon receiving fin message */
                rreq->datatype |= LCP_DATATYPE_UNPACK;
                rc = lcp_recv_am_rput(rreq);
                break;
        case LCP_RNDV_GET:
                /* For RGET, data is unpacked upon completion */
                rreq->state.comp = (lcr_completion_t) {
                        .comp_cb = lcp_request_complete_am_rget
                };
                rc = lcp_mem_unpack(rreq->ctx, &rreq->state.rmem, 
                                    hdr + 1, length);
                if (rc < 0) {
                        goto err;
                }
                rc = lcp_recv_am_rget(rreq);
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown protocol in recv");
                rc = MPC_LOWCOMM_ERROR;
                break;
        }

err:
        return rc;
}

/* ============================================== */
/* Rendez-vous recv completion callback           */
/* ============================================== */

/**
 * @brief Complete a rget rendez-vous
 * 
 * @param comp completion
 */
void lcp_request_complete_am_rget(lcr_completion_t *comp)
{
        int payload_size;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, state.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_ep_h ep;
                lcp_ep_ctx_t *ctx_ep;
                uint64_t puid = lcp_get_process_uid(req->ctx->process_uid,
                                                    req->recv.tag.src_pid);
                /* Get LCP endpoint */
                HASH_FIND(hh, req->ctx->ep_ht, &puid, 
                          sizeof(uint64_t), ctx_ep);
                assert(ctx_ep);

                ep = ctx_ep->ep;

                payload_size = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                                    MPC_LOWCOMM_RFIN_MESSAGE,
                                                    lcp_rndv_fin_pack,
                                                    req);
                if (payload_size < 0) {
                        goto err;
                }

                if (req->datatype & LCP_DATATYPE_DERIVED) {
                        lcp_datatype_unpack(req->ctx, req, req->datatype,
                                            NULL, req->state.pack_buf, 
                                            req->recv.send_length);
                }

                lcp_request_complete(req);
        }
err:
        return;
}

/* ============================================== */
/* Recv                                           */
/* ============================================== */

/**
 * @brief Receive a rget message.
 * 
 * @param req message request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_recv_am_rget(lcp_request_t *req)
{
        int rc, i, num_used_ifaces = 0;
        size_t remaining, offset;
        size_t frag_length, length;
        lcp_mem_h rmem = req->state.rmem; 
        lcp_ep_h ep;
        lcp_ep_ctx_t *ctx_ep;
        lcr_rail_attr_t attr;
        _mpc_lowcomm_endpoint_t *lcr_ep;

        mpc_common_debug_info("LCP: recv am rget src=%d, tag=%d",
                              req->recv.tag.src_tid, req->recv.tag.tag);

        uint64_t puid = lcp_get_process_uid(req->ctx->process_uid,
                                            req->recv.tag.src_pid);
        /* Get LCP endpoint if exists */
        //FIXME: do this twice, one here and one in rndv_matched
        HASH_FIND(hh, req->ctx->ep_ht, &puid, sizeof(uint64_t), ctx_ep);
        if (ctx_ep == NULL) {
                rc = lcp_ep_create(req->ctx, &ep, puid, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not create ep after match.");
                        goto err;
                }
        } else {
                ep = ctx_ep->ep;
        }

        //NOTE: for now on AM datapath, connection map and memory map sent
        //      should be equal because, for each transfert, all transport
        //      endpoints are always connected and memory is pinned on all NICS.
        assert(MPC_BITMAP_AND(ep->conn_map, rmem->bm));

        /* Set starting address */
        void *start = req->datatype & LCP_DATATYPE_CONTIGUOUS ? 
                req->recv.buffer : req->state.pack_buf;

        req->state.remaining = remaining = req->recv.send_length;
        offset               = 0;

        while (remaining > 0) {
                req->state.cc = lcp_ep_get_next_cc(ep);
                if (!MPC_BITMAP_GET(rmem->bm, req->state.cc)) {
                        continue;
                }
                lcr_ep = ep->lct_eps[req->state.cc];
                //FIXME: for now, because homogeneity is forced, all transports
                //       will have the same attributes. So no need to reload for
                //       each.
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_get_zcopy;
                length = remaining < frag_length ? remaining : frag_length;

                rc = lcp_send_do_get_zcopy(lcr_ep,
                                           (uint64_t)start + offset, 
                                           offset, 
                                           &(rmem->mems[req->state.cc]),
                                           length,
                                           &(req->state.comp));
                //TODO: implement no resource return call and progress send call

                offset += length; remaining -= length;
        }

        lcp_mem_delete(rmem);
err:
        return rc;
}

/**
 * @brief Receive a rput request : receive rndv, send ack, receive write, receive fin.
 * 
 * @param req message request
 * @return int MPC_LOWCOMM_SUCCESS
 */
int lcp_recv_am_rput(lcp_request_t *req)
{
        int rc, payload;
        lcp_ep_h ep;
        lcp_ep_ctx_t *ctx_ep;
        lcp_request_t *ack;

        mpc_common_debug_info("LCP: recv am rput src=%d, tag=%d",
                              req->recv.tag.src_tid, req->recv.tag.tag);
        //FIXME: using context process_uid is not so intuitive
        uint64_t puid = lcp_get_process_uid(req->ctx->process_uid, 
                                            req->recv.tag.src_pid);
        /* Get endpoint */
        //FIXME: redundant in am handler
        HASH_FIND(hh, req->ctx->ep_ht, &puid, sizeof(uint64_t), ctx_ep);
        if (ctx_ep == NULL) {
                rc = lcp_ep_create(req->ctx, &ep, puid, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not create ep after match.");
                        goto err;
                }
        } else {
                ep = ctx_ep->ep;
        }

        /* Register memory on which the put will be done */
        lcp_mem_create(req->ctx, &(req->state.lmem));

        /* Register and pack memory pin context that will be sent to remote */
        //FIXME: revise how the memory is registered by sender and receiver...
        /* Set starting address */
        void *start = req->datatype & LCP_DATATYPE_CONTIGUOUS ? 
                req->recv.buffer : req->state.pack_buf;
        //NOTE: for now, memory bitmap from sender receiver are identical and
        //      equal to be endpoint connection bitmap.
        req->state.lmem->bm = ep->conn_map;
        rc = lcp_mem_reg_from_map(req->ctx, 
                                  req->state.lmem,
                                  req->state.lmem->bm,
                                  start, req->recv.send_length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Prepare ack request to send back memory registration contexts */
        lcp_request_create(&ack);
        if (ack == NULL) {
                goto err;
        }
        ack->super = req;
        // initialize pending request. Ack id is request id.
        if (lcp_pending_create(req->ctx->pend, req, req->msg_id) == NULL) {
                mpc_common_debug_error("LCP: could not add pending message");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        /* delete from pending list on completion */
        req->flags |= LCP_REQUEST_DELETE_FROM_PENDING; 

        // send an active message using buffer copy from the endpoint ep
        payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                       MPC_LOWCOMM_ACK_RDV_MESSAGE,
                                       lcp_rtr_rput_pack,
                                       ack);
        if (payload < 0) {
                mpc_common_debug_error("LCP: error sending ack rdv message");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        rc = MPC_LOWCOMM_SUCCESS;

        lcp_request_complete(ack);
err:
        return rc;
}

/* ============================================== */
/* Handlers                                       */
/* ============================================== */

//NOTE: flags could be used to avoid duplicating functions for
//      AM and TAG

/**
 * @brief Callback for rput procedure.
 * 
 * @param arg lcp context
 * @param data rendez-vous header
 * @param length length of header
 * @param flags message flags
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_rndv_am_rput_handler(void *arg, void *data,
                            size_t length, unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_rndv_hdr_t *hdr = data;
        lcp_request_t *req;
        lcp_unexp_ctnr_t *ctnr;
        lcp_task_h task = NULL;

        lcp_context_task_get(ctx, hdr->base.dest_tid, &task);  
        if (task == NULL) {
                mpc_common_debug_error("LCP: could not find task with tid=%d",
                                       hdr->base.dest_tid);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        LCP_TASK_LOCK(task);
        mpc_common_debug_info("LCP: recv rput header src=%d, msg_id=%llu",
                              hdr->base.src_tid, hdr->msg_id);

        req = lcp_match_prq(task->prq_table, hdr->base.comm, 
                            hdr->base.tag, hdr->base.src_tid);
        if (req != NULL) {
                mpc_common_debug_info("LCP: matched rndv exp handler req=%p, comm_id=%lu, " 
                                      "tag=%d, src=%lu.", req, req->recv.tag.comm, 
                                      req->recv.tag.tag, req->recv.tag.src_tid);
                LCP_TASK_UNLOCK(task); //NOTE: unlock context to enable endpoint creation.
                rc = lcp_rndv_matched(req, hdr, length - sizeof(*hdr), 
                                      LCP_RNDV_PUT);
        } else {
                lcp_request_init_unexp_ctnr(&ctnr, hdr, length, LCP_RECV_CONTAINER_UNEXP_RPUT);
                lcp_append_umq(task->umq_table, (void *)ctnr, hdr->base.comm, 
                               hdr->base.tag, hdr->base.src_tid);
                LCP_TASK_UNLOCK(task);
                rc = MPC_LOWCOMM_SUCCESS;
        }

err:
        return rc;
}

/**
 * @brief callback for acknowledgement message 
 * 
 * @param arg context
 * @param data header
 * @param size size of header
 * @param flags unused
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
static int lcp_ack_am_tag_handler(void *arg, void *data,
                                  size_t size, 
                                  __UNUSED__ unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_ack_hdr_t *hdr = data;
        lcp_mem_h rmem = NULL;
        lcp_request_t *req;

        /* Retrieve request */
        req = lcp_pending_get_request(ctx->pend, hdr->msg_id);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not find ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        mpc_common_debug_info("LCP: recv tag ack header src=%d, "
                              "msg_id=%llu.", req->send.tag.dest_tid, 
                              hdr->msg_id);

        rc = lcp_mem_unpack(req->ctx, &rmem,  hdr + 1, 
                            size - sizeof(lcp_ack_hdr_t));
        if (rc < 0) {
                goto err;
        }

        rc = lcp_send_rput_common(req, rmem);

        //FIXME: remote key should be stored in case of NO_RESOURCE
        lcp_mem_delete(rmem);
err:
        return rc;
}

/**
 * @brief Callback for rget procedure
 * 
 * @param arg lcp context
 * @param data rendez-vous header
 * @param length length of header
 * @param flags message flags
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
static int lcp_rndv_am_rget_handler(void *arg, void *data,
                                    size_t length, 
                                    unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_rndv_hdr_t *hdr = data;
        lcp_request_t *req;
        lcp_unexp_ctnr_t *ctnr;
        lcp_task_h task = NULL;

        lcp_context_task_get(ctx, hdr->base.dest_tid, &task);  
        if (task == NULL) {
                mpc_common_debug_error("LCP: could not find task with tid=%d",
                                       hdr->base.dest_tid);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        LCP_TASK_LOCK(task);
        mpc_common_debug_info("LCP: recv rget header src=%d, msg_id=%llu",
                              hdr->base.src_tid, hdr->msg_id);

        req = lcp_match_prq(task->prq_table, hdr->base.comm, 
                            hdr->base.tag, hdr->base.src_tid);
        if (req != NULL) {
                mpc_common_debug_info("LCP: matched rndv exp handler req=%p, comm_id=%llu, " 
                                      "tag=%d, src=%llu.", req, req->recv.tag.comm, 
                                      req->recv.tag.tag, req->recv.tag.src_tid);
                LCP_TASK_UNLOCK(task); //NOTE: unlock context to enable endpoint creation.
                rc = lcp_rndv_matched(req, hdr, length - sizeof(lcp_rndv_hdr_t), 
                                      LCP_RNDV_GET);
        } else {
                lcp_request_init_unexp_ctnr(&ctnr, hdr, length, LCP_RECV_CONTAINER_UNEXP_RGET);
                lcp_append_umq(task->umq_table, (void *)ctnr, hdr->base.comm, 
                               hdr->base.tag, hdr->base.src_tid);
                LCP_TASK_UNLOCK(task);
                rc = MPC_LOWCOMM_SUCCESS;
        }

err:
        return rc;
}

/**
 * @brief Callback for rendez-vous finalization.
 * 
 * @param arg lcp context
 * @param data rendez-vous header
 * @param length header length
 * @param flags message flags
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
static int lcp_rndv_am_fin_handler(void *arg, void *data,
                                   __UNUSED__ size_t length, 
                                   __UNUSED__ unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_ack_hdr_t *hdr = data;
        lcp_request_t *req;


        /* Retrieve request */
        req = lcp_pending_get_request(ctx->pend, hdr->msg_id);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not find ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        mpc_common_debug_info("LCP: recv rfin header msg_id=%llu, req=%p",
                              hdr->msg_id, req);

        lcp_mem_deregister(req->ctx, req->state.lmem);

        if (req->datatype & LCP_DATATYPE_DERIVED) {
                if (req->datatype & LCP_DATATYPE_UNPACK) {
                        lcp_datatype_unpack(req->ctx, req, req->datatype,
                                            NULL, req->state.pack_buf, 
                                            req->state.lmem->length);
                }
                /* Free buffer allocated by rndv */
                sctk_free(req->state.pack_buf);
        }

	LCP_CONTEXT_LOCK(ctx);
        lcp_request_complete(req);

	LCP_CONTEXT_UNLOCK(ctx);
err:
        return rc;
}

/**
 * rendez-vous callbacks registering 
 */
LCP_DEFINE_AM(MPC_LOWCOMM_ACK_RDV_MESSAGE, lcp_ack_am_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RGET_MESSAGE, lcp_rndv_am_rget_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RPUT_MESSAGE, lcp_rndv_am_rput_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RFIN_MESSAGE, lcp_rndv_am_fin_handler, 0);
