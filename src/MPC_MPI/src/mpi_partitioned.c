#include "mpi_partitioned.h"

#include "mpc_mpi_internal.h"

#include "lcp.h"
#include "lcp_common.h"

extern lcp_context_h lcp_ctx_loc;

static inline void build_partition_flags_map(const uint64_t buffer,
                                            size_t length,
                                            int recv_partitions, 
                                            int send_partitions,
                                            prtd_map_t **map_p)
{
        int i = 0, j = 0, pstart = 0;
        size_t r_size, s_size;
        uint64_t r_off, s_off;
        prtd_map_t *map = sctk_malloc(recv_partitions * sizeof(prtd_map_t));

        r_size = length / (size_t)recv_partitions;
        s_size = length / (size_t)send_partitions;
        for (r_off = buffer; r_off < buffer + length; r_off += r_size) {
                for (s_off = buffer; s_off < buffer + length; s_off += s_size) {
                        if (r_off >= s_off && r_off < s_off + s_size && !pstart) {
                                map[i].p_id_start = j;
                                pstart = 1;
                        }

                        if (r_off + r_size <= s_off + s_size 
                            || j == send_partitions-1) {
                                map[i].p_id_end = j;
                                j = 0;
                                pstart = 0;
                                break;
                        }
                        j++;
                }
                i++;
        }

        *map_p = map;
}

int mpi_partitioned_complete_fin(mpc_lowcomm_request_t *req) 
{
        mpi_partitioned_t *prtd = mpc_container_of(req, mpi_partitioned_t,
                                                   fin_req);
        MPI_internal_request_t *mpi_req = 
                mpc_container_of(prtd, MPI_internal_request_t,
                                 partitioned);

        prtd->fin_req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
        mpi_req->req.completion_flag  = MPC_LOWCOMM_MESSAGE_DONE;

        return 0;
}

int mpi_partitioned_complete_tag(mpc_lowcomm_request_t *req) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep;
        lcp_task_h task;
        lcp_context_h ctx = lcp_ctx_loc;
        size_t cflags_size;
        void *cflags_tmp_buf, *prtd_tmp_buf;
        size_t cflags_tmp_buf_size, prtd_tmp_buf_size;
        mpi_partitioned_t *prtd = mpc_container_of(req, mpi_partitioned_t,
                                                   tag_req);
        MPI_internal_request_t *mpi_req = 
                mpc_container_of(prtd, MPI_internal_request_t,
                                 partitioned);
 
        task = lcp_context_task_get(ctx, mpc_common_get_task_rank());
        if (task == NULL) {
                mpc_common_debug_fatal("LCP: task %d not fount", 
                                       mpc_common_get_task_rank());
                return MPC_LOWCOMM_ERROR;
        }

        switch (req->request_type) {
        case REQUEST_SEND:
                req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
                break;
        case REQUEST_RECV:
                /* Flags map is used to commit completion on the receiver side,
                 * especially when the number of partitions is different between
                 * the send and the receive sides. */
                build_partition_flags_map((uint64_t)prtd->buf, prtd->length,
                                          prtd->partitions, 
                                          prtd->recv.send_partitions, 
                                          &prtd->recv.map);

                /* Allocate and registercompletion flags memory on which sender
                 * will Put the completion flags whenever the partition has been
                 * sent and received */
                cflags_size = prtd->recv.send_partitions * sizeof(int);
                prtd->recv.cflags_buf = sctk_malloc(cflags_size);

                rc = lcp_mem_register(ctx, &prtd->rkey_cflags, 
                                      prtd->recv.cflags_buf, cflags_size);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request type "
                                               "error");
                        return rc;
                }

                /* Pack both rkey for application memory and for completion
                 * flags */
                rc = lcp_mem_pack(ctx, prtd->rkey_cflags, &cflags_tmp_buf,
                                  &cflags_tmp_buf_size);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request could "
                                               "not pack rkey");
                        return rc;
                }

                rc = lcp_mem_pack(ctx, prtd->rkey_prtd, &prtd_tmp_buf,
                                  &prtd_tmp_buf_size);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request could "
                                               "not pack rkey");
                        return rc;
                }
                
                /* Merge them into one buffer */
                prtd->recv.rkeys_buf_size = prtd_tmp_buf_size + cflags_tmp_buf_size;
                prtd->recv.rkeys_buf = sctk_malloc(prtd->recv.rkeys_buf_size);
                memcpy(prtd->recv.rkeys_buf, cflags_tmp_buf, cflags_tmp_buf_size);
                memcpy(prtd->recv.rkeys_buf + cflags_tmp_buf_size, prtd_tmp_buf, 
                       prtd_tmp_buf_size);

                /* Release buffers provided by lcp once they have been copied */
                lcp_mem_release_rkey_buf(cflags_tmp_buf);
                lcp_mem_release_rkey_buf(prtd_tmp_buf);

                /* Send both rkeys to origin process */
                lcp_request_param_t param_rkeys = { 0 };
                ep = lcp_ep_get(ctx, prtd->rkey_req.header.destination);
                rc = lcp_tag_send_nb(ep, task, prtd->recv.rkeys_buf, 
                                     prtd->recv.rkeys_buf_size,
                                     &prtd->rkey_req, &param_rkeys);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request could "
                                               "not send rkey");
                        return rc;
                }

                /* Post FIN message */
                lcp_request_param_t param = (lcp_request_param_t) {
                        .tag_info = &mpi_req->req.tag_info,
                };
                rc = lcp_tag_recv_nb(task, &prtd->fin, sizeof(int), 
                                     &mpi_req->partitioned.fin_req, 
                                     &param);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request could not post recv");
                        return rc;
                }

                break;
        default:
                mpc_common_debug_error("PRTD: wrong request type");
                break;
        }

        return 0;
}

int mpi_partitioned_complete_rkey(mpc_lowcomm_request_t *req) 
{
        lcp_context_h ctx = lcp_ctx_loc;
        size_t rkey_length;
        mpi_partitioned_t *prtd = mpc_container_of(req, mpi_partitioned_t,
                                                   rkey_req);
        switch (req->request_type) {
        case REQUEST_RECV:
                rkey_length = req->tag_info.length / 2;
                
                /* Extract first remote key: cflags */
                lcp_mem_unpack(ctx, &prtd->rkey_cflags, prtd->send.rkeys_buf,
                               rkey_length);

                lcp_mem_unpack(ctx, &prtd->rkey_prtd, prtd->send.rkeys_buf + 
                               rkey_length, rkey_length);

                break;
        case REQUEST_SEND:
                sctk_free(prtd->recv.rkeys_buf);
                break;
        default:
                break;
        }
        req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;

        return 0;
}

int mpi_partitioned_complete_cflags(mpc_lowcomm_request_t *req) 
{
        UNUSED(req);
        return 0;
}

int mpi_partitioned_complete_put_partition(mpc_lowcomm_request_t *req) 
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep;
        lcp_task_h task;
        lcp_context_h ctx = lcp_ctx_loc;
        MPI_internal_request_t *mpi_req = mpc_container_of(req, MPI_internal_request_t,
                                                           req);

        task = lcp_context_task_get(ctx, mpc_common_get_task_rank());
        if (task == NULL) {
                mpc_common_debug_fatal("LCP: task %d not fount", 
                                       mpc_common_get_task_rank());
                return MPC_LOWCOMM_ERROR;
        }
        /* Set remote completion flags to 1. Used by mpi_parrived to know if */
        for (i=0; i<mpi_req->partitioned.partitions; i++) {
                if (mpi_req->partitioned.send.send_cflags[i] == 1) {
                        lcp_request_param_t param = {
                                .on_rma_completion = mpi_partitioned_complete_cflags,
                                .request           = &mpi_req->req,
                                .flags             = LCP_REQUEST_USER_REQUEST
                        };
                        ep = lcp_ep_get(ctx, mpi_req->req.header.destination);
                        rc = lcp_put_nb(ep, task, &mpi_req->partitioned.complete, 
                                        sizeof(int), i * sizeof(int), 
                                        mpi_req->partitioned.rkey_prtd, 
                                        &param);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                mpc_common_debug_error("MPI: partitioned request "
                                                       "put cflags error");
                                return rc;
                        }
                        /* reset completion flags of corresponding partition to
                         * 0 */
                        mpi_req->partitioned.send.send_cflags[i] = 0;
                }
        }

        if (OPA_fetch_and_decr_int(&mpi_req->partitioned.counter) == 0) {
                lcp_request_param_t param = { 0 };
                mpi_req->partitioned.fin = 1;
                ep = lcp_ep_get(ctx, mpi_req->req.header.destination);
                rc = lcp_tag_send_nb(ep, task, &mpi_req->partitioned.fin, 
                                     sizeof(int), &mpi_req->partitioned.fin_req, 
                                     &param);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request send "
                                               "fin error");
                        return rc;
                }
        }

        return rc;
}

int mpi_partitioned_complete_partitioned(mpc_lowcomm_request_t *req) 
{
        req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;        

        return 0;
}

int mpi_psend_init(const void *buf, int partitions, int count, 
                   MPI_Datatype datatype, int dest, int tag, 
                   MPI_Comm comm, MPI_internal_request_t *req)
{
	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
                //mpi_check_tag_send(tag, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	if(dest == MPC_PROC_NULL)
	{
		req->freeable         = 0;
		req->is_active        = 0;
		req->req.request_type = REQUEST_NULL;

		req->partitioned.buf         = (void *)buf;
                req->partitioned.partitions  = partitions;
		req->partitioned.count       = 0;
		req->partitioned.datatype    = datatype;
		req->partitioned.dest_source = MPC_PROC_NULL;
		req->partitioned.tag         = MPI_ANY_TAG;
	}
	else
	{
		req->freeable         = 0;
		req->is_active        = 0;
                req->req.request_type = REQUEST_SEND;
                req->is_partitioned   = 1; req->is_persistent = 0;

                size_t dt_size;
                _mpc_cl_type_size(datatype, &dt_size);
                req->partitioned.length      = count * dt_size;

		req->partitioned.buf         = (void *)buf;
                req->partitioned.partitions  = partitions;
		req->partitioned.count       = count;
		req->partitioned.datatype    = datatype;
		req->partitioned.dest_source = dest;
		req->partitioned.tag         = tag;
		req->partitioned.comm        = comm;
                req->partitioned.complete    = 1;
                req->partitioned.send.rkeys_size = 2*1024;
                req->partitioned.send.send_cflags = sctk_malloc(partitions *
                                                                sizeof(int));
                memset(req->partitioned.send.send_cflags, 0, partitions);

                OPA_store_int(&req->partitioned.counter, partitions-1);
                mpc_lowcomm_request_init_struct(&req->req, comm, REQUEST_SEND,
                                                mpc_common_get_task_rank(), dest,
                                                tag, mpi_partitioned_complete_put_partition);
                mpc_lowcomm_request_init_struct(&req->partitioned.tag_req, comm, 
                                                REQUEST_SEND, mpc_common_get_task_rank(), 
                                                dest, tag, mpi_partitioned_complete_tag);
                mpc_lowcomm_request_init_struct(&req->partitioned.rkey_req, comm, 
                                                REQUEST_RECV, dest, mpc_common_get_task_rank(), 
                                                tag, mpi_partitioned_complete_rkey);
                mpc_lowcomm_request_init_struct(&req->partitioned.fin_req, comm, REQUEST_SEND,
                                                mpc_common_get_task_rank(), dest,
                                                tag, mpi_partitioned_complete_fin);
	}

	return MPI_SUCCESS;
}

int mpi_precv_init(void *buf, int partitions, int count, 
                    MPI_Datatype datatype, int source, int tag, 
                    MPI_Comm comm, MPI_internal_request_t *req)
{
	{
		int size;
		mpi_check_comm(comm);
		mpi_check_type(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		//mpi_check_tag(tag, comm);
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank(source, size, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	req->freeable         = 0;
	req->is_active        = 0;
	req->req.request_type = REQUEST_RECV;
        req->is_partitioned   = 1; req->is_persistent = 0;

        size_t dt_size;
        _mpc_cl_type_size(datatype, &dt_size);
        req->partitioned.length      = count * dt_size;

        req->partitioned.buf         = (void *)buf;
        req->partitioned.partitions  = partitions;
        req->partitioned.count       = count;
        req->partitioned.datatype    = datatype;
        req->partitioned.dest_source = source;
        req->partitioned.tag         = tag;
        req->partitioned.comm        = comm;
        mpc_lowcomm_request_init_struct(&req->req, comm, REQUEST_RECV,
                                        source, mpc_common_get_task_rank(),
                                        tag, mpi_partitioned_complete_partitioned);
        mpc_lowcomm_request_init_struct(&req->partitioned.tag_req, comm, 
                                        REQUEST_RECV, source, 
                                        mpc_common_get_task_rank(), 
                                        tag, mpi_partitioned_complete_tag);
        mpc_lowcomm_request_init_struct(&req->partitioned.rkey_req, comm, 
                                        REQUEST_SEND, mpc_common_get_task_rank(), 
                                        source, tag, mpi_partitioned_complete_rkey);
        mpc_lowcomm_request_init_struct(&req->partitioned.fin_req, comm, 
                                        REQUEST_RECV, source, 
                                        mpc_common_get_task_rank(), 
                                        tag, mpi_partitioned_complete_fin);

	return MPI_SUCCESS;
}

int mpi_pstart(MPI_internal_request_t *req) 
{
        int rc; 
        lcp_ep_h ep;
        lcp_context_h ctx = lcp_ctx_loc;
        lcp_task_h task;
        mpi_partitioned_t *prtd = &req->partitioned;
        mpc_lowcomm_status_t status;
        lcp_request_param_t param;

        mpc_common_debug_info("PRTD: Start request");

        //FIXME: 
        req->is_active = 1;
        task = lcp_context_task_get(ctx, mpc_common_get_task_rank());
        if (task == NULL) {
                mpc_common_debug_fatal("LCP: task %d not fount", 
                                       mpc_common_get_task_rank());
                return MPC_LOWCOMM_ERROR;
        }

        switch (req->req.request_type) {
        case REQUEST_SEND:
                ep = lcp_ep_get(ctx, prtd->tag_req.header.destination);
                param = (lcp_request_param_t) { 0 };
                rc = lcp_tag_send_nb(ep, task, &req->partitioned.partitions, 
                                     sizeof(int), &prtd->tag_req, 
                                     &param);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request type error");
                        return rc;
                }
                //FIXME: set 1024 as max size of packed rkeys
                prtd->send.rkeys_buf = sctk_malloc(prtd->send.rkeys_size); 
                if (prtd->send.rkeys_buf == NULL) {
                        mpc_common_debug_error("MPI: partitioned request could not "
                                               "allocate rkey buf");
                        return 1;
                }

                param = (lcp_request_param_t) {
                        .tag_info = &prtd->rkey_req.tag_info,

                };
                rc = lcp_tag_recv_nb(task, prtd->send.rkeys_buf, prtd->send.rkeys_size,
                                     &prtd->rkey_req, &param);
                
                mpc_lowcomm_wait(&prtd->rkey_req, &status);

                break;
        case REQUEST_RECV:
                rc = lcp_mem_register(ctx, &prtd->rkey_prtd, prtd->buf, prtd->length);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request type error");
                        return rc;
                }

                param = (lcp_request_param_t) {
                        .tag_info = &prtd->tag_req.tag_info,

                };
                rc = lcp_tag_recv_nb(task, &prtd->recv.send_partitions,
                                     sizeof(int), &prtd->tag_req, &param);
                break;
        default:
                rc = MPC_LOWCOMM_ERROR;
                mpc_common_debug_error("MPI: partitioned request type error");
                break;
        }
                
        return MPI_SUCCESS;
}

int mpi_pready(int partition, MPI_internal_request_t *req)
{
        int rc; 
        lcp_ep_h ep;
        lcp_context_h ctx = lcp_ctx_loc;
        lcp_task_h task;
        mpi_partitioned_t *prtd = &req->partitioned;
        void *local_addr;
        uint64_t remote_offset;
        size_t partition_length;

        task = lcp_context_task_get(ctx, mpc_common_get_task_rank());
        if (task == NULL) {
                mpc_common_debug_fatal("LCP: task %d not fount", 
                                       mpc_common_get_task_rank());
                return MPC_LOWCOMM_ERROR;
        }
        
        /* Compute local address, remote offset and send length */ 
        remote_offset = partition * prtd->length / prtd->partitions;
        local_addr = prtd->buf + remote_offset;
        if (prtd->length % prtd->partitions == 0) {
               partition_length = prtd->length / prtd->partitions;
        } else {
               partition_length = prtd->length / prtd->partitions +
                       prtd->length % prtd->partitions;
        }

        /* Set local completion flags to 1 so that corresponding put is
         * performed upon completion. Must be set beforehand in case of
         * immediate completion. */
        prtd->send.send_cflags[partition] = 1;

        /* Send partition i */
        lcp_request_param_t param = {
                .on_rma_completion = mpi_partitioned_complete_put_partition,
                .request = &req->req,
                .flags   = LCP_REQUEST_USER_REQUEST
        };
        ep = lcp_ep_get(ctx, prtd->tag_req.header.destination);
        rc = lcp_put_nb(ep, task, local_addr, partition_length, remote_offset, 
                        prtd->rkey_prtd, &param);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("MPI: partitioned request type error");
                return rc;
        }


        return MPI_SUCCESS;
}

int mpi_parrived(int partition, MPI_internal_request_t *req, int *flags_p)
{
        int i, flags;
        prtd_map_t p_map = req->partitioned.recv.map[partition];
        
        flags = 1;
        for (i=p_map.p_id_start; i<=(int)p_map.p_id_end; i++) {
                if (!req->partitioned.recv.cflags_buf[i]) {
                        flags = 0;
                        break;
                }
        }

        *flags_p = flags;

        return MPI_SUCCESS;
}

