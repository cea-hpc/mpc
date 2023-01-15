#include "mpc_mpi_internal.h"
#include "mpc_common_rank.h"

#include "lcp.h"

void mpi_partitioned_complete_send_tag(mpc_lowcomm_request_t *req) 
{
        
}

void mpi_partitioned_complete_recv_rkey(mpc_lowcomm_request_t *req) 
{
        
}

void mpi_partitioned_complete_partitioned(mpc_lowcomm_request_t *req) 
{
        
}

int PMPI_Psend_init(const void *buf, int partitions, int count, 
                    MPI_Datatype datatype, int dest, int tag, 
                    MPI_Comm comm, MPI_Request *request)
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

	MPI_internal_request_t *req;

	req = __sctk_new_mpc_request_internal(request, 
                                              __sctk_internal_get_MPC_requests());
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

		req->partitioned.buf         = (void *)buf;
                req->partitioned.partitions  = partitions;
		req->partitioned.count       = count;
		req->partitioned.datatype    = datatype;
		req->partitioned.dest_source = dest;
		req->partitioned.tag         = tag;
		req->partitioned.comm        = comm;
                mpc_lowcomm_request_init_struct(&req->req, comm, REQUEST_SEND,
                                                mpc_common_get_task_rank(), dest,
                                                tag);
                mpc_lowcomm_request_init_struct(&req->partitioned.tag_req, comm, 
                                                REQUEST_SEND, mpc_common_get_task_rank(), 
                                                dest, tag);
                mpc_lowcomm_request_init_struct(&req->partitioned.rkey_req, comm, 
                                                REQUEST_SEND, mpc_common_get_task_rank(), 
                                                dest, tag);
	}

	return MPI_SUCCESS;
}

int PMPI_Precv_init(void *buf, int partitions, int count, MPI_Datatype datatype, 
                    int source, int tag, MPI_Comm comm, MPI_Request *request)
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

	MPI_internal_request_t *req;

	req                   = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests() );
	req->freeable         = 0;
	req->is_active        = 0;
	req->req.request_type = REQUEST_RECV;

        req->partitioned.buf         = (void *)buf;
        req->partitioned.partitions  = partitions;
        req->partitioned.count       = count;
        req->partitioned.datatype    = datatype;
        req->partitioned.dest_source = source;
        req->partitioned.tag         = tag;
        req->partitioned.comm        = comm;
        mpc_lowcomm_request_init_struct(&req->req, comm, REQUEST_SEND,
                                        source, mpc_common_get_task_rank(),
                                        tag, mpi_partitioned_complete_partitioned);
        mpc_lowcomm_request_init_struct(&req->partitioned.tag_req, comm, 
                                        REQUEST_SEND, source, 
                                        mpc_common_get_task_rank(), 
                                        tag, mpi_partitioned_complete_send_tag);
        mpc_lowcomm_request_init_struct(&req->partitioned.rkey_req, comm, 
                                        REQUEST_SEND, source, 
                                        mpc_common_get_task_rank(), 
                                        tag, mpi_partitioned_complete_recv_rkey);

	return MPI_SUCCESS;
}

int mpi_pstart(MPI_internal_request_t *req) 
{
        int rc; 
        lcp_ep_h ep;
        lcp_context_h ctx = lcp_context_get();
        mpi_partitioned_t prtd = req->partitioned;
        lcp_request_param_t param;

        switch (req->req.request_type) {
        case REQUEST_SEND:
                lcp_ep_get(lcp_context_get(), prtd.tag_req.header.destination,
                           &ep);
                rc = lcp_tag_send_nb(ep, &req->partitioned.partitions, 
                                     sizeof(int), &prtd.tag_req, 
                                     &param);

                break;
        case REQUEST_RECV:
                rc = lcp_mem_register(ctx, &prtd.rkey_prt, prtd.buf, prtd.count);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request type error");
                        return rc;
                }

                rc = lcp_mem_register(ctx, &prtd.rkey_cflags, prtd.buf, prtd.count);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI: partitioned request type error");
                        return rc;
                }

                param = (lcp_request_param_t) {
                        .recv_info = &prtd.tag_req.recv_info,

                };
                rc = lcp_tag_recv_nb(lcp_context_get(), prtd.recv.send_partitions,
                                     sizeof(int), prtd.tag_req, &param);
                break;
        default:
                rc = MPC_LOWCOMM_ERROR;
                mpc_common_debug_error("MPI: partitioned request type error");
                break;
        }
                
}
