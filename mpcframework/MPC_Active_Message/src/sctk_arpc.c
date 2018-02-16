#include <arpc.h>
#include <mpi.h>
#include <sctk_allocator.h>
#include <sctk_debug.h>

#define MPI_ARPC_TAG  1000
#define __arpc_print_ctx(ctx, format,...) do { \
	sctk_warning("[%d,S:%d,C:%d] "format, ctx->dest,ctx->srvcode, ctx->rpcode, ##__VA_ARGS__);} while(0)

int arpc_emit_call(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void* response, size_t*resp_size)
{
	__arpc_print_ctx(ctx, "Send req=%p (sz:%llu) <--> resp=%p (sz: %llu)", input, req_size, response, *resp_size);

	MPI_Send( &ctx->rpcode , 1 , MPI_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	MPI_Send( &ctx->srvcode , 1 , MPI_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );

	MPI_Send( &req_size , 1 , MPI_LONG_LONG_INT, ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	
	if(req_size >0){

		MPI_Send( input , req_size , MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	}
	MPI_Recv( resp_size , 1 , MPI_LONG_LONG_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );

	if(*resp_size > 0)
	{
		MPI_Recv( response , *(int*)resp_size , MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	}

	return 1;
}

int arpc_recv_call(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void* response, size_t* resp_size)
{
	__arpc_print_ctx(ctx, "Recv req=%p (sz:%llu) <--> resp=%p (sz: %llu)", input, req_size, response, *resp_size);


	
	return 1;
}


int arpc_polling_request(sctk_arpc_context_t* ctx)
{
	void * request = NULL;
	size_t req_size = 0;
	void * response = NULL;
	size_t resp_size = 0;


	int i = 0;
	MPI_Recv( &ctx->rpcode , 1 , MPI_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	MPI_Recv( &ctx->srvcode , 1 , MPI_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );

	MPI_Recv( &req_size , 1 , MPI_LONG_LONG_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	if(req_size > 0)
	{
		request = malloc(req_size);
		MPI_Recv( request , req_size, MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	}
	
	arpc_recv_call(ctx, request, req_size, &response, &resp_size);
	
	response =malloc(resp_size);

	MPI_Send( &resp_size , 1 , MPI_LONG_LONG_INT , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	
	if(resp_size > 0)
	{
		MPI_Send( response , resp_size , MPI_BYTE , ctx->dest , MPI_ARPC_TAG , MPI_COMM_WORLD );
	}

	return 0;
}
