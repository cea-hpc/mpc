#include <arpc.h>
#include <mpi.h>
#include <sctk_alloc.h>
#include <sctk_debug.h>
#include <sctk_atomics.h>

#define MPI_ARPC_TAG  1000
#define __arpc_print_ctx(ctx, format,...) do { \
	sctk_warning("[%d,S:%d,C:%d] "format, ctx->dest,ctx->srvcode, ctx->rpcode, ##__VA_ARGS__);} while(0)

#define MAX_STATIC_ARPC_SIZE 4096


#define SCTK_SIZEOF_INTERNAL_CTX (4 * sizeof(int))
typedef struct sctk_arpc_mpi_ctx_s
{
	int rpcode;
	int srvcode;
	int next_tag;
	int msize;
	char raw[MAX_STATIC_ARPC_SIZE];
} sctk_arpc_mpi_ctx_t;

sctk_atomics_int atomic_tag = OPA_INT_T_INITIALIZER(0);

/**
 * Trigger a RPC send call.
 * \param[in] ctx the ARPC context
 * \param[in] input the request
 * \param[in] req_size request size
 * \param[in,out] response the address of the output buffer
 * \param[out] resps_ize response size
 * \return 0 if ok, 1 otherwise
 */
int arpc_emit_call(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size)
{
	/*__arpc_print_ctx(ctx, "Send req=%p (sz:%llu) <--> resp=%p (sz: %llu)", input, req_size, response, *resp_size);*/

	sctk_arpc_mpi_ctx_t mpi_ctx;
	int next_tag = sctk_atomics_fetch_and_incr_int(&atomic_tag);

	mpi_ctx.rpcode = ctx->rpcode;
	mpi_ctx.srvcode = ctx->srvcode;
	mpi_ctx.msize = req_size;
	mpi_ctx.next_tag = next_tag;

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
		MPI_Send(input, req_size, MPI_BYTE, ctx->dest, mpi_ctx.next_tag, MPI_COMM_WORLD );
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
int arpc_recv_call(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response_addr, size_t* resp_size)
{
	/* this function will jump into C++ code */
	return arpc_c_to_cxx_converter(ctx, input, req_size, response_addr, resp_size );
}

int arpc_polling_request(sctk_arpc_context_t* ctx)
{
	void *request = NULL, *response = NULL;
	size_t req_size = 0, resp_size = 0;
	int next_tag;
	int ret;

	MPI_Status st;

	sctk_arpc_mpi_ctx_t mpi_ctx;
	memset(&mpi_ctx, 0 , sizeof(sctk_arpc_mpi_ctx_t));

	MPI_Recv( &mpi_ctx , sizeof(sctk_arpc_mpi_ctx_t) , MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , &st );
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

		MPI_Recv(request , req_size, MPI_BYTE , ctx->dest , mpi_ctx.next_tag , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	}
	
	((char*)request)[req_size] = '\0';
	
	/* forward to the effective RPC call */
	ret = arpc_recv_call(ctx, request, req_size, &response, &resp_size);
	
	/* now, if there is something to return */
	if(response != NULL)
	{
		/*__arpc_print_ctx(ctx, "Recv req=%p (sz:%llu) <--> resp=%p (sz: %llu)", request, req_size, response, resp_size);*/
		mpi_ctx.msize = resp_size;
		if(mpi_ctx.msize < MAX_STATIC_ARPC_SIZE - 1)
		{
			memcpy(mpi_ctx.raw, response, resp_size);
			MPI_Send( &mpi_ctx , resp_size + SCTK_SIZEOF_INTERNAL_CTX , MPI_BYTE , ctx->dest , next_tag , MPI_COMM_WORLD);
		}
		else
		{
			MPI_Send( &mpi_ctx , SCTK_SIZEOF_INTERNAL_CTX , MPI_BYTE , ctx->dest , next_tag , MPI_COMM_WORLD);
			MPI_Send( response , resp_size , MPI_BYTE , ctx->dest , next_tag , MPI_COMM_WORLD );
		}
		sctk_free(response);
	}
	return ret;
}
