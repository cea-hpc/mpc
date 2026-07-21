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
/* #   - REYNIER Florian    florian.reynier@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

#include "mpc_lowcomm_msg.h"
#include "mpc_lowcomm_workshare.h"
#include <mpc_common_rank.h>
#include <sctk_alloc.h>
#include "lowcomm_request.h"
#include <mpc_common_progress.h>


mpc_lowcomm_request_t __request_null;

mpc_lowcomm_request_t * mpc_lowcomm_request_null(void)
{
	return &__request_null;
}

struct mpi_request_info mpi_req_info = { 0 };

int mpc_lowcomm_pass_mpi_request_info(size_t request_size, void (*request_init_func)(void *))
{
	mpi_req_info.size = request_size;
	mpi_req_info.request_init_func = request_init_func;

	return MPC_LOWCOMM_SUCCESS;
}

int _mpc_lowcomm_request_send_complete(int status, void *request, size_t length)
{
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

	assert(req->completion_flag != MPC_LOWCOMM_MESSAGE_DONE);
	if (status != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal("COMM: request failed with status=%d.", status);
	}

	// FIXME: MPI standard does not require the size, tag and src fields to
	//       be filled but some MPC test in the CI requires send, so set
	//       them from header info.
	req->status_error     = status;
	req->recv_info.length = length;

	if (req->flags & MPC_LOWCOMM_REQUEST_CALLBACK)
	{
		req->request_complete(req);
	}

	req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;

	return MPC_LOWCOMM_SUCCESS;
}

void mpc_lowcomm_request_initialize(void *request)
{
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

	req->is_allocated = MPC_LOWCOMM_REQUEST_ALLOC;
	mpc_lowcomm_request_set_null(request, 0);

	if (mpi_req_info.request_init_func)
	{
		mpi_req_info.request_init_func(req + 1);
	}
}

/********************************************************************/
/*Message pack creation                                             */
/********************************************************************/


void mpc_lowcomm_request_pack(void *request, void *buffer)
{
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

	switch (req->dt_type)
	{
	case MPC_LOWCOMM_MESSAGE_PACK:
	{
		size_t i;
		size_t j;
		size_t size;
		size = req->header.msg_size;

		if (size > 0)
		{
			for (i = 0; i < req->dt.count; i++)
			{
				for (j = 0; j < req->dt.list.std[i].count; j++)
				{
					size = (req->dt.list.std[i].ends[j]
					        - req->dt.list.std[i].begins[j]
					        + 1) * req->dt.list.std[i].elem_size;
					memcpy(buffer, (( char * )(req->dt.list.std[i].addr))
						+ req->dt.list.std[i].begins[j]
						* req->dt.list.std[i].elem_size, size);
					buffer += size;
				}
			}
		}

		break;
	}

	case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
	{
		size_t i;
		size_t j;
		size_t size;
		size = req->header.msg_size;

		if (size > 0)
		{
			for (i = 0; i < req->dt.count; i++)
			{
				for (j = 0; j < req->dt.list.absolute[i].count; j++)
				{
					size = (req->dt.list.absolute[i].ends[j]
					        - req->dt.list.absolute[i].begins[j]
					        + 1) * req->dt.list.absolute[i].elem_size;
					memcpy(buffer, (( char * )(req->dt.list.absolute[i].addr))
						+ req->dt.list.absolute[i].begins[j]
						* req->dt.list.absolute[i].elem_size, size);
					buffer += size;
				}
			}
		}

		break;
	}

	default:
		not_reachable();
	}
}

void mpc_lowcomm_request_unpack(void *request, void *buffer)
{
	size_t i, j;
	size_t size;
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

	switch (req->dt_type)
	{
	case MPC_LOWCOMM_REQUEST_PACK:
		size = req->header.msg_size;
		if (size > 0)
		{
			size_t total = 0;
			char   skip  = 0;

			for (i = 0; i < req->dt.count; i++)
			{
				for (j = 0; (j < req->dt.list.std[i].count) && !skip; j++)
				{
					size = (req->dt.list.std[i].ends[j]
					        - req->dt.list.std[i].begins[j]
					        + 1) * req->dt.list.std[i].elem_size;

					mpc_common_nodebug("%p - %p \n", req->dt.list.std[i].begins[j],
						req->dt.list.std[i].ends[j]);
					if (total + size > req->header.msg_size)
					{
						skip = 1;
						size = req->header.msg_size - total;
					}

					memcpy(((char *)(req->dt.list.std[i].addr))
						+ req->dt.list.std[i].begins[j]
						* req->dt.list.std[i].elem_size, buffer, size);
					buffer += size;
					total  += size;
					assume(total <= req->header.msg_size);
				}
			}
		}
		break;

	case MPC_LOWCOMM_REQUEST_PACK_ABSOLUTE:
		size = req->header.msg_size;
		if (size > 0)
		{
			size_t total = 0;
			char   skip  = 0;

			for (i = 0; i < req->dt.count; i++)
			{
				for (j = 0; (j < req->dt.list.absolute[i].count) && !skip; j++)
				{
					size = (req->dt.list.absolute[i].ends[j]
					        - req->dt.list.absolute[i].begins[j]
					        + 1) * req->dt.list.absolute[i].elem_size;

					mpc_common_nodebug("%p - %p \n", req->dt.list.std[i].begins[j],
						req->dt.list.std[i].ends[j]);
					if (total + size > req->header.msg_size)
					{
						skip = 1;
						size = req->header.msg_size - total;
					}

					memcpy((( char * )(req->dt.list.absolute[i].addr))
						+ req->dt.list.absolute[i].begins[j]
						* req->dt.list.absolute[i].elem_size, buffer, size);
					buffer += size;
					total  += size;
					assume(total <= req->header.msg_size);
				}
			}
		}
		break;

	default:
		not_reachable();
	}
}

#define SCTK_PACK_REALLOC_STEP 10
void mpc_lowcomm_request_add_pack(mpc_lowcomm_request_t *req, void *adr,
                                  const unsigned int nb_items,
                                  const size_t elem_size,
                                  unsigned long *begins,
                                  unsigned long *ends)
{
	size_t step;

	req->dt_type  = MPC_LOWCOMM_REQUEST_PACK;
	req->dt_magic = 0xDDDDDDDD;
	if (req->dt.count >= req->dt.max_count)
	{
		req->dt.max_count += SCTK_PACK_REALLOC_STEP;
		req->dt.list.std   =
			sctk_realloc(req->dt.list.std,
				req->dt.max_count
				* sizeof(mpc_lowcomm_request_pack_std_list_t));
	}

	step = req->dt.count;
	req->dt.list.std[step].count     = nb_items;
	req->dt.list.std[step].begins    = begins;
	req->dt.list.std[step].ends      = ends;
	req->dt.list.std[step].addr      = adr;
	req->dt.list.std[step].elem_size = elem_size;
	req->dt.count++;
}

void mpc_lowcomm_request_add_pack_absolute(mpc_lowcomm_request_t *req,
                                           const void *adr, const unsigned int nb_items,
                                           const size_t elem_size,
                                           long *begins,
                                           long *ends)
{
	size_t step;

	req->dt_type  = MPC_LOWCOMM_REQUEST_PACK_ABSOLUTE;
	req->dt_magic = 0xDDDDDDDD;
	if (req->dt.count >= req->dt.max_count)
	{
		req->dt.max_count    += SCTK_PACK_REALLOC_STEP;
		req->dt.list.absolute =
			sctk_realloc(req->dt.list.absolute,
				req->dt.max_count
				* sizeof(mpc_lowcomm_request_pack_absolute_list_t));
	}

	step = req->dt.count;
	req->dt.list.absolute[step].count     = nb_items;
	req->dt.list.absolute[step].begins    = begins;
	req->dt.list.absolute[step].ends      = ends;
	req->dt.list.absolute[step].addr      = adr;
	req->dt.list.absolute[step].elem_size = elem_size;
	req->dt.count++;
}

static void __mpc_comm_perform_req_wfv(void *a)
{
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)a;

	if (( volatile int )req->completion_flag != MPC_LOWCOMM_MESSAGE_DONE)
	{
		mpc_common_progress();
		MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	}
}

void mpc_lowcomm_perform_idle(volatile int *data, int value,
                              void (*func)(void *), void *arg)
{
	if (*data == value)
	{
		return;
	}

#ifndef MPC_Threads
		while (*data != value)
		{
			func(arg);
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
		}
#else
		mpc_thread_wait_for_value_and_poll(data, value, func, arg);
#endif
}

static void (*__egreq_poll_trampoline)(void) = NULL;

void mpc_lowcomm_egreq_poll_set_trampoline(void (*trampoline)(void))
{
	__egreq_poll_trampoline = trampoline;
}

static inline void __egreq_poll(void *arg)
{
	UNUSED(arg);
	if (__egreq_poll_trampoline)
	{
		(__egreq_poll_trampoline)();
	}
}

int request_is_null_or_cancelled(mpc_lowcomm_request_t *request)
{
	if (!request)
	{
		return 1;
	}

	if (mpc_lowcomm_request_is_null(request))
	{
		return 1;
	}

	if (request->completion_flag == MPC_LOWCOMM_MESSAGE_CANCELED)
	{
		return 1;
	}

	if (request->request_type == REQUEST_NULL)
	{
		return 1;
	}

	return 0;
}

int mpc_lowcomm_request_wait(mpc_lowcomm_request_t *request)
{
	if (request_is_null_or_cancelled(request))
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	if (mpc_lowcomm_request_get_source(request) == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_set_null(request, 1);
	}

#ifdef MPC_MPI
		if (request->request_type == REQUEST_GENERALIZED)
		{
			mpc_lowcomm_perform_idle(( int * )&(request->completion_flag),
				MPC_LOWCOMM_MESSAGE_DONE, __egreq_poll, NULL);
		}
		else
#endif
	{
		int trials = 0;
		do
		{
			mpc_common_progress();
			trials++;
		} while ((request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE) && (trials < 16));

		if (request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE)
		{
			mpc_lowcomm_perform_idle(
				( int * )&(request->completion_flag), MPC_LOWCOMM_MESSAGE_DONE,
				(void (*)(void *)) __mpc_comm_perform_req_wfv,
				request);
		}
	}

	return MPC_LOWCOMM_SUCCESS;
}
