#include <mpc_mpi.h>

#include <mpc_mpi_internal.h>

#ifdef MPC_Threads
#include <mpc_thread.h>
#endif

#ifdef MPC_LOWCOMM_PROTOCOL
#include <lcp_common.h>
#endif

int mpc_lowcomm_prequest_complete(mpc_lowcomm_request_t *preq)
{
        int i, done = 1;
        MPI_internal_request_t *master = (MPI_internal_request_t *)preq->pointer_to_source_request;
        preq->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;

        for (i=0; i<master->partitioned.partitions; i++) {
                if (master->partitioned.p_reqs[i].completion_flag != MPC_LOWCOMM_MESSAGE_DONE) {
                        done = 0;
                        break;
                }
        }

        if (done) {
                master->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
        }

        return MPC_LOWCOMM_SUCCESS;
}

int PMPI_Psend_init(const void *buf, int partitions, int count, MPI_Datatype datatype, int dest,
                    int tag, MPI_Comm comm, MPI_Request *request)
{
	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	MPI_internal_request_t *req;

	req = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests() );
	mpc_common_nodebug("new request %d", *request);
	if(dest == MPC_PROC_NULL)
	{
		req->freeable         = 0;
		req->is_active        = 0;
		req->req.request_type = REQUEST_NULL;

		req->partitioned.buf         = (void *)buf;
		req->partitioned.count       = 0;
		req->partitioned.datatype    = datatype;
		req->partitioned.dest_source = MPC_PROC_NULL;
		req->partitioned.tag         = MPI_ANY_TAG;
		req->partitioned.comm        = comm;
		req->partitioned.op          = MPC_MPI_PERSISTENT_SEND_INIT;
	}
	else
	{
		req->freeable         = 0;
		req->is_active        = 0;
                req->is_partitioned   = 1;
		req->req.request_type = REQUEST_SEND;

		req->partitioned.buf         = (void *)buf;
		req->partitioned.count       = count;
		req->partitioned.datatype    = datatype;
		req->partitioned.dest_source = dest;
		req->partitioned.tag         = tag;
		req->partitioned.comm        = comm;
		req->partitioned.op          = MPC_MPI_PERSISTENT_SEND_INIT;
                req->partitioned.partitions  = partitions;
                req->partitioned.p_reqs      = sctk_malloc(partitions * sizeof(mpc_lowcomm_request_t));
                req->partitioned.id          = (int)lcp_rand_uint64();

                for (int i=0; i< partitions; i++) {
                        req->partitioned.p_reqs[i].pointer_to_source_request = (void *)req;
                }
	}
	return MPI_SUCCESS;
}

int PMPI_Precv_init(const void *buf, int partitions, int count, MPI_Datatype datatype, int dest,
                    int tag, MPI_Comm comm, MPI_Request *request)
{
	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);

		if(count != 0) {
			mpi_check_buf(buf, comm);
		}
	}

	MPI_internal_request_t *req;

	req = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests() );
	mpc_common_nodebug("new request %d", *request);
	if(dest == MPC_PROC_NULL) {
		req->freeable         = 0;
		req->is_active        = 0;
		req->req.request_type = REQUEST_NULL;

		req->partitioned.buf         = (void *)buf;
		req->partitioned.count       = 0;
		req->partitioned.datatype    = datatype;
		req->partitioned.dest_source = MPC_PROC_NULL;
		req->partitioned.tag         = MPI_ANY_TAG;
		req->partitioned.comm        = comm;
		req->partitioned.op          = MPC_MPI_PERSISTENT_RECV_INIT;
	} else {
		req->freeable         = 0;
		req->is_active        = 0;
                req->is_partitioned   = 1;
		req->req.request_type = REQUEST_RECV;

		req->partitioned.buf         = (void *)buf;
		req->partitioned.count       = count;
		req->partitioned.datatype    = datatype;
		req->partitioned.dest_source = dest;
		req->partitioned.tag         = tag;
		req->partitioned.comm        = comm;
		req->partitioned.op          = MPC_MPI_PERSISTENT_RECV_INIT;
                req->partitioned.partitions  = partitions;
                req->partitioned.p_reqs      = sctk_malloc(partitions * sizeof(mpc_lowcomm_request_t));
                req->partitioned.id          = (int)lcp_rand_uint64();

                for (int i=0; i< partitions; i++) {
                        req->partitioned.p_reqs[i].request_completion_fn = mpc_lowcomm_prequest_complete;
                        req->partitioned.p_reqs[i].pointer_to_source_request = (void *)req;
                }
	}

	return MPI_SUCCESS;
}

int PMPI_Pready(int partition, MPI_Request request)
{
        MPI_Comm comm           = MPI_COMM_WORLD;
        int res                 = MPI_ERR_INTERN;
        int tag                 = 0;
        size_t                  partition_size, dt_size;
        void                   *buf;
        mpc_lowcomm_request_t   lc_req;
        MPI_internal_request_t *req;
        MPI_request_struct_t   *requests;

        requests = __sctk_internal_get_MPC_requests();
        req      = __sctk_convert_mpc_request_internal(&request, requests);
        lc_req   = req->partitioned.p_reqs[partition]; 

        /* compute matching tag */
        tag |= (req->partitioned.id & 0xffffffu);
        tag |= (tag << 8);
        tag |= (partition & 0xffu);

        /* compute offset and count */
        if (partition == req->partitioned.partitions - 1) {
                /* if last partition */
                partition_size = req->partitioned.count / req->partitioned.partitions +
                        req->partitioned.count % req->partitioned.partitions;
        } else {
                partition_size = req->partitioned.count / req->partitioned.partitions;
        }
        _mpc_cl_type_size(req->partitioned.datatype, &dt_size);
        buf = req->partitioned.buf + partition_size * dt_size * partition; 

        res = _mpc_cl_isend(buf, partition_size, 
                            req->partitioned.datatype, 
                            req->partitioned.dest_source,
                            tag,
                            req->partitioned.comm,
                            &lc_req);

        MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Parrived(MPI_Request request, int partition, int *flags)
{
        mpc_lowcomm_request_t   lc_req;
        MPI_internal_request_t *req;
        MPI_request_struct_t   *requests;

        requests = __sctk_internal_get_MPC_requests();
        req      = __sctk_convert_mpc_request_internal(&request, requests);
        lc_req   = req->partitioned.p_reqs[partition]; 

        if (lc_req.completion_flag == MPC_LOWCOMM_MESSAGE_DONE) {
                *flags = 1;
        } else {
                *flags = 0;
        }

        return MPI_SUCCESS;
}

