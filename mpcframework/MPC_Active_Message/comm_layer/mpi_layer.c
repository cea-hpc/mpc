/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpi_layer.h>

#ifdef MPC_Active_Message
#include <sctk_alloc.h>
#include <mpc_common_asm.h>
#include <sctk_debug.h>
#else
#include <stdlib.h>
#include <assert.h>
#define sctk_malloc malloc
#define sctk_free free
#define sctk_assert assert

#ifndef OPA_INT_T_INITIALIZER
#define OPA_INT_T_INITIALIZER(v) v
#endif
#define sctk_atomics_int int
#define sctk_atomics_fetch_and_incr_int(v) ((*v)++)
#endif

static sctk_atomics_int atomic_tag = OPA_INT_T_INITIALIZER(0);

int arpc_init_mpi(int nb_srv)
{
	return 0;
}

/**
 * Trigger a RPC send call.
 * \param[in] ctx the ARPC context
 * \param[in] input the request
 * \param[in] req_size request size
 * \param[in,out] response the address of the output buffer
 * \param[out] resps_ize response size
 * \return 0 if ok, 1 otherwise
 */
int arpc_emit_call_mpi(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size)
{
	sctk_arpc_mpi_ctx_t mpi_ctx;
	int next_tag = sctk_atomics_fetch_and_incr_int(&atomic_tag);

	mpi_ctx.rpcode = ctx->rpcode;
	mpi_ctx.srvcode = ctx->srvcode;
	mpi_ctx.msize = req_size;
	mpi_ctx.next_tag = next_tag;
	//__arpc_print_ctx(ctx, "Send req=%p (sz:%llu) <--> resp=%p (sz: %llu) TAG = %d", input, req_size, response, *resp_size, next_tag);

	/* first, send the context main infos */
	/* if request is null, don't send it ! */
	if(req_size < MAX_STATIC_ARPC_SIZE - 1)
	{
		memcpy(mpi_ctx.raw, input, req_size);
		MPI_Send( &mpi_ctx , req_size + SCTK_SIZEOF_INTERNAL_CTX , MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	}
	else
	{
		MPI_Send(&mpi_ctx, SCTK_SIZEOF_INTERNAL_CTX, MPI_BYTE, ctx->dest, MPI_ARPC_TAG, MPI_COMM_WORLD );
		MPI_Send((void*)input, req_size, MPI_BYTE, ctx->dest, mpi_ctx.next_tag, MPI_COMM_WORLD );
	}

	/* Now, wait for the request to complete.
	 * Note that if the RPC does not need a return value, the call is not blocking from here */
	if(response != NULL)
	{
		sctk_assert(resp_size);
		memset(&mpi_ctx, 0, sizeof(sctk_arpc_mpi_ctx_t)); /* bo be sure */
		MPI_Status st;
		/* wait for the response buffer size */
		MPI_Recv( &mpi_ctx, sizeof(sctk_arpc_mpi_ctx_t) , MPI_BYTE , ctx->dest , next_tag , MPI_COMM_WORLD , &st );

		*resp_size = mpi_ctx.msize;
		*response = sctk_malloc(mpi_ctx.msize + 1);
		((char*)*response)[*resp_size] = '\0';

		if(mpi_ctx.msize < MAX_STATIC_ARPC_SIZE - 1)
		{
			memcpy((char*)*response, (char*)mpi_ctx.raw, mpi_ctx.msize);
		}
		else
		{
			MPI_Recv( *response , mpi_ctx.msize, MPI_BYTE,  ctx->dest , next_tag , MPI_COMM_WORLD , MPI_STATUS_IGNORE);
		}
	}
	return 0;
}

/**
 * Recv a RPC call to process.
 * \param[in] ctx the RPC context
 * \param[in] input the received request
 * \param[in] req_size the size of the request
 * \param[out] response_addr a pointer to the buffer to return
 * \param[out] resp_size a pointer to the size of the response
 * \return 0 if ok, 1 otherwise
 */
int arpc_recv_call_mpi(sctk_arpc_context_t* ctx, const void* request, size_t req_size, void** response_addr, size_t* resp_size)
{
	int next_tag;
	int ret;
	
	MPI_Status st;
	sctk_arpc_mpi_ctx_t mpi_ctx;
	memset(&mpi_ctx, 0 , sizeof(sctk_arpc_mpi_ctx_t));

	MPI_Recv( &mpi_ctx , sizeof(sctk_arpc_mpi_ctx_t) , MPI_BYTE , MPI_ANY_SOURCE, MPI_ARPC_TAG , MPI_COMM_WORLD , &st );
	ctx->dest = st.MPI_SOURCE;
	ctx->rpcode = mpi_ctx.rpcode;
	ctx->srvcode = mpi_ctx.srvcode;
	next_tag = mpi_ctx.next_tag;
	req_size = mpi_ctx.msize;

	if(req_size < MAX_STATIC_ARPC_SIZE - 1)
	{
		request = mpi_ctx.raw;
	}
	else
	{
		request = sctk_malloc(req_size + 1);
		MPI_Recv((void*)request , req_size, MPI_BYTE , ctx->dest , mpi_ctx.next_tag , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	}
	
	((char*)request)[req_size] = '\0';
	
	/* forward to the effective RPC call */
	sctk_assert(response_addr && resp_size);
	*response_addr = NULL;
	*resp_size = 0;
	ret = arpc_c_to_cxx_converter(ctx, request, req_size, response_addr, resp_size );

	if(request != mpi_ctx.raw)
		sctk_free((void*)request);
	
	/* now, if there is something to return */
	if(*response_addr != NULL)
	{
		//__arpc_print_ctx(ctx, "Recv req=%p (sz:%llu) <--> resp=%p (sz: %llu) TAG = %d", request, req_size, *response_addr, *resp_size, next_tag);
		mpi_ctx.msize = *resp_size;
		if(mpi_ctx.msize < MAX_STATIC_ARPC_SIZE - 1)
		{
			memcpy(mpi_ctx.raw, (*response_addr), mpi_ctx.msize);
			MPI_Send( &mpi_ctx , mpi_ctx.msize + SCTK_SIZEOF_INTERNAL_CTX , MPI_BYTE , ctx->dest , next_tag , MPI_COMM_WORLD);
		}
		else
		{
			MPI_Send( &mpi_ctx , SCTK_SIZEOF_INTERNAL_CTX , MPI_BYTE , ctx->dest , next_tag , MPI_COMM_WORLD);
			MPI_Send( *response_addr , mpi_ctx.msize , MPI_BYTE , ctx->dest , next_tag , MPI_COMM_WORLD );
		}
		sctk_free(*response_addr);
	}
	return ret;
}

int arpc_polling_request_mpi(sctk_arpc_context_t* ctx)
{
	void *request = NULL, *response = NULL;
	size_t req_size = 0, resp_size = 0;

	return arpc_recv_call(ctx, request, req_size, &response, &resp_size);

}

int arpc_register_service_mpi(void* cxx_pool, int srvcode)
{
	/* nothing to do */
	return -1;
}

int arpc_free_response_mpi(void* addr)
{
	return -1;
}
