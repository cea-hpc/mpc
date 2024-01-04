#include "mpc_lowcomm_msg.h"
#include "am.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ffi.h>
#include <dlfcn.h>
#include <sctk_alloc.h>

#include <mpc_lowcomm_communicator.h>
#include <mpc_common_debug.h>

/* TODO need more types */

static inline size_t mpc_lowcomm_am_type_t_extent(mpc_lowcomm_am_type_t type)
{
	switch(type)
	{
		case MPC_LOWCOMM_AM_NULL:
			return 0;

		case MPC_LOWCOMM_AM_INT:
			return sizeof(int);

		case MPC_LOWCOMM_AM_FLOAT:
			return sizeof(float);

		case MPC_LOWCOMM_AM_DOUBLE:
			return sizeof(double);
	}

	fprintf(stderr, "TYPE NOT SUPPORTED YET\n");
	abort();
}

static inline char *mpc_lowcomm_am_type_t_str(mpc_lowcomm_am_type_t type)
{
	switch(type)
	{
		case MPC_LOWCOMM_AM_NULL:
			return "NULL";

		case MPC_LOWCOMM_AM_INT:
			return "int";

		case MPC_LOWCOMM_AM_FLOAT:
			return "float";

		case MPC_LOWCOMM_AM_DOUBLE:
			return "double";
	}

	fprintf(stderr, "TYPE NOT SUPPORTED YET\n");
	abort();
}

static inline ffi_type *ffi_type_from_mpc_lowcomm_am_type_t(mpc_lowcomm_am_type_t type)
{
	switch(type)
	{
		case MPC_LOWCOMM_AM_NULL:
			return &ffi_type_void;

		case MPC_LOWCOMM_AM_INT:
			return &ffi_type_sint;

		case MPC_LOWCOMM_AM_FLOAT:
			return &ffi_type_float;

		case MPC_LOWCOMM_AM_DOUBLE:
			return &ffi_type_double;
	}

	fprintf(stderr, "TYPE NOT SUPPORTED YET\n");
	abort();
}

struct active_message_rpc_desc
{
	short 				  poison;
	int                   source;
	int                   response_tag;
	char                  end_of_rpc;
	char                  function[MPC_LOWCOMM_AM_MAX_FN_LEN];
	int                   send_count;
	mpc_lowcomm_am_type_t send_types[MPC_LOWCOMM_AM_MAX_SEND_CNT];
	mpc_lowcomm_am_type_t recv_type;
	char                  data[0];
};

void active_message_rpc_desc_print(struct active_message_rpc_desc *rpcd)
{
	printf("==========================\n");
	printf("Poison '%x (%s)'\n", rpcd->poison, (rpcd->poison == 0x77)?"OK":"NOK");
	printf("Calling '%s'\n", rpcd->function);
	printf("In parameters (%d) :\n", rpcd->send_count);
	int i;
	for(i = 0; i < rpcd->send_count; i++)
	{
		printf("\t%s\n", mpc_lowcomm_am_type_t_str(rpcd->send_types[i]) );
	}
	printf("Recv type: %s\n", mpc_lowcomm_am_type_t_str(rpcd->recv_type) );
	printf("From %d:%d\n", rpcd->source, rpcd->response_tag);
	printf("End of RPC flag %hd\n", (short)rpcd->end_of_rpc);
	printf("==========================\n");
}

struct active_message_rpc_desc *active_message_rpc_desc_eof()
{
	static __thread struct active_message_rpc_desc eof;

	memset(&eof, 0, sizeof(struct active_message_rpc_desc) );

	eof.poison = 0x77;
	eof.end_of_rpc = 1;

	return &eof;
}

#define MPC_LOWCOMM_RPC_TAG 16

static inline struct active_message_rpc_desc *active_message_rpc_desc_init(mpc_lowcomm_am_ctx_t ctx,
																		   char *function,
                                                                           mpc_lowcomm_am_type_t *send_types,
                                                                           void **buffers,
                                                                           int send_count,
                                                                           mpc_lowcomm_am_type_t recv_type,
                                                                           int end_of_rpc,
                                                                           size_t *out_overall_size,
																		   void * tmp_buffer,
																		   size_t tmp_buffer_size,
                                                                           int *to_free)
{
	/* MPC_LOWCOMM_RPC_TAG is reserved for RPC notification
	 * all the rest is increasing tags */
	static __thread int             expected_response_tag = MPC_LOWCOMM_RPC_TAG + 1;

	/* Compute overall size */
	size_t overall_size = sizeof(struct active_message_rpc_desc);

	int i;

	for(i = 0; i < send_count; i++)
	{
		overall_size += mpc_lowcomm_am_type_t_extent(send_types[i]);
	}

	struct active_message_rpc_desc *rpcd = NULL;

	if(overall_size < tmp_buffer_size)
	{
		rpcd     = (struct active_message_rpc_desc *)tmp_buffer;
		*to_free = 0;
		/* /!\ This means we do not yield in the middle before SENDING !! */
	}
	else
	{
		rpcd     = (struct active_message_rpc_desc *)sctk_malloc(overall_size * sizeof(char) );
		*to_free = 1;
	}

	*out_overall_size = overall_size;

	if(!rpcd)
	{
		perror("sctk_malloc");
		abort();
	}

	rpcd->response_tag = expected_response_tag;

	expected_response_tag = (expected_response_tag + 1) % (1024*1024) + MPC_LOWCOMM_RPC_TAG + 1;

	rpcd->source = mpc_lowcomm_get_comm_rank(ctx->active_message_comm);

	rpcd->end_of_rpc = end_of_rpc;

	if(strlen(function) >= MPC_LOWCOMM_AM_MAX_FN_LEN)
	{
		fprintf(stderr, "Function name too long\n");
		abort();
	}

	snprintf(rpcd->function, MPC_LOWCOMM_AM_MAX_FN_LEN, "%s", function);
	rpcd->send_count = send_count;

	if(send_count >= MPC_LOWCOMM_AM_MAX_SEND_CNT)
	{
		fprintf(stderr, "Maximum parameter count is %d\n", MPC_LOWCOMM_AM_MAX_SEND_CNT);
		abort();
	}

	memcpy(rpcd->send_types, send_types, send_count * sizeof(mpc_lowcomm_am_type_t) );
	rpcd->recv_type = recv_type;

	/* Is is now time to pack */

	size_t offset = 0;

	for(i = 0; i < send_count; i++)
	{
		size_t ext = mpc_lowcomm_am_type_t_extent(send_types[i]);
		memcpy(rpcd->data + offset, buffers[i], ext);
		offset += ext;
	}

	rpcd->poison = 0x77;

	return rpcd;
}

#define MPC_LOWCOMM_AM_MAX_PAYLOAD    (2 * 1024 * 1024)
#define MPC_LOWCOMM_AM_MAX_RESP       1024

static inline void *_resolve_fname(char *fname)
{
	static __thread char *pfname = NULL;
	static __thread void *pfn    = NULL;

	if(pfname)
	{
		if(!strcmp(pfname, fname) )
		{
			return pfn;
		}
	}

	free(pfname);
	pfname = strdup(fname);
	pfn    = dlsym(NULL, fname);

	return pfn;
}

static inline int active_message_process_rpc(struct active_message_rpc_desc *rpcd, void *result_buffer)
{
	assume(rpcd->poison == 0x77);
	ffi_cif cif;
	void *  arg_ffi_v[MPC_LOWCOMM_AM_MAX_SEND_CNT];

	{
		//TODO: cache CIFs
		ffi_type *arg_ffi[MPC_LOWCOMM_AM_MAX_SEND_CNT];

		int i;

		size_t offset = 0;

		for(i = 0; i < rpcd->send_count; i++)
		{
			arg_ffi[i]   = ffi_type_from_mpc_lowcomm_am_type_t(rpcd->send_types[i]);
			arg_ffi_v[i] = rpcd->data + offset;
			offset      += mpc_lowcomm_am_type_t_extent(rpcd->send_types[i]);
		}

		ffi_type *ret_ffi = ffi_type_from_mpc_lowcomm_am_type_t(rpcd->recv_type);

		ffi_status ret = ffi_prep_cif(&cif,
		                              FFI_DEFAULT_ABI,
		                              rpcd->send_count,
		                              ret_ffi,
		                              arg_ffi);

		if(ret != FFI_OK)
		{
			return -1;
		}
	}

	void *fn = NULL;

	{
		//TODO: cache fn
		fn = _resolve_fname(rpcd->function);
		//fn = dlsym(NULL, rpcd->function);

		if(!fn)
		{
			active_message_rpc_desc_print(rpcd);
			bad_parameter("Could not resolve function '%s'", rpcd->function);
		}
	}

	ffi_call(&cif, fn, result_buffer, arg_ffi_v);

	/* Break the rpcd */
	rpcd->poison = 0x11;

	return 0;
}

void *active_message_progress_loop(void *pctx)
{
	struct mpc_lowcomm_am_ctx_s * ctx = (struct mpc_lowcomm_am_ctx_s *)pctx;
	void *tmp_buffer = sctk_malloc(MPC_LOWCOMM_AM_MAX_PAYLOAD * sizeof(char) );
	char  result_buffer[MPC_LOWCOMM_AM_MAX_RESP];

	char *debug = getenv("MPC_LOWCOMM_AM_DEBUG");

	while(1)
	{
		size_t msg_size = 0;

		mpc_lowcomm_request_t req;
		mpc_lowcomm_status_t status;
		mpc_lowcomm_irecv(MPC_ANY_SOURCE, tmp_buffer, MPC_LOWCOMM_AM_MAX_PAYLOAD, MPC_LOWCOMM_RPC_TAG, ctx->active_message_comm, &req);
		mpc_lowcomm_wait(&req, &status);

		msg_size = status.size;

		if(MPC_LOWCOMM_AM_MAX_PAYLOAD < msg_size)
		{
			fprintf(stderr, "Message payload is too large (%ld / %d)\n", msg_size, MPC_LOWCOMM_AM_MAX_PAYLOAD);
			abort();
		}

		struct active_message_rpc_desc *rpcd = (struct active_message_rpc_desc *)tmp_buffer;


		if(debug)
		{
			active_message_rpc_desc_print(rpcd);
		}

		if(rpcd->end_of_rpc)
		{
			break;
		}

		if(MPC_LOWCOMM_AM_MAX_RESP <= mpc_lowcomm_am_type_t_extent(rpcd->recv_type) )
		{
			fprintf(stderr, "Message response is too large (%ld / %d)\n", mpc_lowcomm_am_type_t_extent(rpcd->recv_type), MPC_LOWCOMM_AM_MAX_RESP);
			abort();
		}

		active_message_process_rpc(rpcd, &result_buffer);

		/* No need to answer if response is NULL */
		if(rpcd->recv_type != MPC_LOWCOMM_AM_NULL)
		{
			mpc_lowcomm_send(rpcd->source,
			                 result_buffer,
			                 mpc_lowcomm_am_type_t_extent(rpcd->recv_type),
			                 rpcd->response_tag,
			                 ctx->active_message_comm);
		}
	}

	sctk_free(tmp_buffer);
	mpc_common_debug("RPC THREAD Leaving");

	return NULL;
}

pthread_t acc_progress_thread;

mpc_lowcomm_am_ctx_t mpc_lowcomm_am_init()
{
	struct mpc_lowcomm_am_ctx_s * ctx = sctk_malloc(sizeof(struct mpc_lowcomm_am_ctx_s));
	ctx->active_message_comm = mpc_lowcomm_communicator_dup(MPC_COMM_WORLD);
	pthread_create(&ctx->acc_progress_thread, NULL, active_message_progress_loop, (void*)ctx);
	mpc_lowcomm_barrier(ctx->active_message_comm);

	return ctx;
}

int mpc_lowcomm_am_release(mpc_lowcomm_am_ctx_t *pctx)
{
	if(!pctx)
	{
		/* Already freed ? */
		return 1;
	}

	mpc_lowcomm_am_ctx_t ctx = *pctx;
	mpc_lowcomm_barrier(ctx->active_message_comm);

	int myself = mpc_lowcomm_get_comm_rank(ctx->active_message_comm);

	struct active_message_rpc_desc *eof = active_message_rpc_desc_eof();

	mpc_lowcomm_send(myself,
	                 eof,
	                 sizeof(struct active_message_rpc_desc),
	                 MPC_LOWCOMM_RPC_TAG,
	                 ctx->active_message_comm);

	pthread_join(ctx->acc_progress_thread, NULL);

	mpc_lowcomm_barrier(ctx->active_message_comm);

	mpc_lowcomm_communicator_free(&ctx->active_message_comm);

	sctk_free(ctx);

	*pctx = NULL;

	return 0;
}


#define MPC_LOWCOMM_AM_STATIC_COMM_BUFF    1024

int mpc_lowcomm_iam(mpc_lowcomm_am_ctx_t ctx,
				    void **buffers,
                    mpc_lowcomm_am_type_t *datatypes,
                    int sendcount,
                    void *recvbuf,
                    mpc_lowcomm_am_type_t recvtype,
                    char *fname,
                    int dest,
                    mpc_lowcomm_request_t *req)
{
	size_t overall_size = 0;
	int    to_free      = 0;

	char static_comm_buff[MPC_LOWCOMM_AM_STATIC_COMM_BUFF];

	struct active_message_rpc_desc *rpcd = active_message_rpc_desc_init(ctx,
																		fname,
	                                                                    datatypes,
	                                                                    buffers,
	                                                                    sendcount,
	                                                                    recvtype,
	                                                                    0,
	                                                                    &overall_size,
																		static_comm_buff,
																		MPC_LOWCOMM_AM_STATIC_COMM_BUFF,
	                                                                    &to_free);

	mpc_lowcomm_send(dest, rpcd, overall_size, MPC_LOWCOMM_RPC_TAG, ctx->active_message_comm);

	/* We only expect a response if we have a return type */
	if(rpcd->recv_type != MPC_LOWCOMM_AM_NULL)
	{
		mpc_lowcomm_irecv(dest,
		                  recvbuf,
		                  mpc_lowcomm_am_type_t_extent(recvtype),
		                  rpcd->response_tag,
		                  ctx->active_message_comm,
		                  req);
	}
	else
	{
		/* Mark as complete */
		req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
	}

	if(to_free)
	{
		sctk_free(rpcd);
	}

	return 0;
}

int mpc_lowcomm_am(mpc_lowcomm_am_ctx_t ctx,
				   void **buffers,
                   mpc_lowcomm_am_type_t *datatypes,
                   int sendcount,
                   void *recvbuf,
                   mpc_lowcomm_am_type_t recvtype,
                   char *fname,
                   int dest)
{
	mpc_lowcomm_request_t req;

	mpc_lowcomm_iam(ctx,
					buffers,
	                datatypes,
	                sendcount,
	                recvbuf,
	                recvtype,
	                fname,
	                dest,
	                &req);
	
	return mpc_lowcomm_wait(&req, MPC_LOWCOMM_STATUS_NULL);
}
