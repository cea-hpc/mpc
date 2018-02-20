#include <arpc.h>
#include <mpi.h>
#include <sctk_alloc.h>
#include <sctk_debug.h>

#define MPI_ARPC_TAG  1000
#define __arpc_print_ctx(ctx, format,...) do { \
	sctk_warning("[%d,S:%d,C:%d] "format, ctx->dest,ctx->srvcode, ctx->rpcode, ##__VA_ARGS__);} while(0)

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

	/* first, send the context main infos */
	MPI_Send( &ctx->rpcode , 1 , MPI_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	MPI_Send( &ctx->srvcode , 1 , MPI_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );

	/* send the request size */
	MPI_Send( &req_size , 1 , MPI_LONG_LONG_INT, ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	
	/* if request is null, don't send it ! */
	if(req_size >0){

		MPI_Send( input , req_size , MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	}

	/* Now, wait for the request to complete.
	 * Note that if the RPC does not need a return value, the call is not blocking from here */
	if(response != NULL)
	{
		sctk_assert(resp_size);
		/* wait for the response buffer size */
		MPI_Recv( resp_size , 1 , MPI_LONG_LONG_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );

	/*__arpc_print_ctx(ctx, "Resp req=%p (sz:%llu) <--> resp=%p (sz: %llu)", input, req_size, response, *resp_size);*/

		/* allocate the response buffer according to spec.
		 * Force '\0' for later use by protobuf
		 */
		*response = sctk_malloc((*resp_size) + 1 );
		((char*)*response)[*resp_size] = '\0';
		MPI_Recv( *response , *(int*)resp_size , MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
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
	int ret;

	/* wait for a request to be received */
	MPI_Recv( &ctx->rpcode , 1 , MPI_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	MPI_Recv( &ctx->srvcode , 1 , MPI_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );

	/* which size is the request ? */
	MPI_Recv( &req_size , 1 , MPI_LONG_LONG_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	if(req_size > 0)
	{
		/* add an extra '\0'.
		 * Protobuf needs it for later use
		 */
		request = sctk_malloc(req_size + 1);
		((char*)request)[req_size] = '\0';

		MPI_Recv( request , req_size, MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	}
	
	/* forward to the effective RPC call */
	ret = arpc_recv_call(ctx, request, req_size, &response, &resp_size);
	
	/* now, if there is something to return */
	if(response != NULL)
	{
		/*__arpc_print_ctx(ctx, "Recv req=%p (sz:%llu) <--> resp=%p (sz: %llu)", request, req_size, response, resp_size);*/
		MPI_Send( &resp_size , 1 , MPI_LONG_LONG_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
		MPI_Send( response , resp_size , MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	}
	return ret;
}
