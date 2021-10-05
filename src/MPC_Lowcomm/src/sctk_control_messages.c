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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_control_messages.h"

#include <stdio.h>

#include <sctk_low_level_comm.h>
#include "communicator.h"

#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <mpc_common_asm.h>
#include <sctk_checksum.h>
#include <reorder.h>
#include <sctk_rail.h>
#include <sctk_alloc.h>
#include <sctk_window.h>
#include <mpc_lowcomm_rdma.h>
#include "sctk_topological_polling.h"

#include <communicator.h>

/************************************************************************/
/* Process Level Messages                                               */
/************************************************************************/

#ifdef MPC_MPI
/* MPI Level win handler */
void mpc_MPI_Win_control_message_handler(void *data, size_t size);

#else
/* Dummy substitute implemenentation */
void mpc_MPI_Win_control_message_handler(__UNUSED__ void *data, __UNUSED__ size_t size)
{
}

#endif

void sctk_control_message_task_level(__UNUSED__ int source_process, __UNUSED__ int source_rank,
                                     __UNUSED__ char subtype, __UNUSED__ char param, __UNUSED__ void *data,
                                     __UNUSED__ size_t size)
{
	switch(subtype)
	{
		// case SCTK_PROCESS_RDMA_CONTROL_MESSAGE:
		// mpc_MPI_Win_control_message_handler(data, size);
		// break;
		default:
			not_implemented();
	}
}

void sctk_control_message_process_level(__UNUSED__ int source_process, __UNUSED__ int source_rank,
                                        char subtype, __UNUSED__ char param, void *data,
                                        __UNUSED__ size_t size)
{
	struct mpc_lowcomm_rdma_window_map_request *  mr   = NULL;
	struct mpc_lowcomm_rdma_window_emulated_RDMA *erma = NULL;

	__UNUSED__ struct sctk_control_message_fence_ctx *         fence = NULL;
	struct mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA *fop   = NULL;
	struct mpc_lowcomm_rdma_window_emulated_CAS_RDMA *         fcas  = NULL;
	int win_id = -1;

	switch(subtype)
	{
		case SCTK_PROCESS_FENCE:
			//fence = (struct sctk_control_message_fence_ctx *)data;
			// sctk_control_message_fence_handler(fence);
			break;

		case SCTK_PROCESS_RDMA_WIN_MAPTO:
			mpc_common_nodebug("Received a MAP remote from %d", source_rank);
			mr = (struct mpc_lowcomm_rdma_window_map_request *)data;
			mpc_lowcomm_rdma_window_map_remote_ctrl_msg_handler(mr);
			break;

		case SCTK_PROCESS_RDMA_WIN_RELAX:
			memcpy(&win_id, data, sizeof(int) );
			mpc_common_nodebug("Received a  WIN relax from %d on %d", source_rank, win_id);
			mpc_lowcomm_rdma_window_relax_ctrl_msg_handler(win_id);
			break;

		case SCTK_PROCESS_RDMA_EMULATED_WRITE:
			erma = (struct mpc_lowcomm_rdma_window_emulated_RDMA *)data;
			mpc_lowcomm_rdma_window_RDMA_emulated_write_ctrl_msg_handler(erma);
			break;

		case SCTK_PROCESS_RDMA_EMULATED_READ:
			erma = (struct mpc_lowcomm_rdma_window_emulated_RDMA *)data;
			mpc_lowcomm_rdma_window_RDMA_emulated_read_ctrl_msg_handler(erma);
			break;

		case SCTK_PROCESS_RDMA_EMULATED_FETCH_AND_OP:
			fop = (struct mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA *)data;
			mpc_lowcomm_rdma_window_RDMA_fetch_and_op_ctrl_msg_handler(fop);
			break;

		case SCTK_PROCESS_RDMA_EMULATED_CAS:
			fcas = (struct mpc_lowcomm_rdma_window_emulated_CAS_RDMA *)data;
			mpc_lowcomm_rdma_window_RDMA_CAS_ctrl_msg_handler(fcas);
			break;

		default:
			not_implemented();
	}
}

/************************************************************************/
/* Control Messages Send Recv                                           */
/************************************************************************/

static void sctk_free_control_messages(__UNUSED__ void *ptr)
{
}

void printpayload(__UNUSED__ void *pl, size_t size)
{
	size_t i;

	mpc_common_debug("======== %ld ========", size);
	for(i = 0; i < size; i++)
	{
		mpc_common_debug("%d = [%hu]  ", i, ( (char *)pl)[i]);
	}
	mpc_common_debug("===================");
}

void __sctk_control_messages_send(int dest, int dest_task,
                                  mpc_lowcomm_ptp_message_class_t message_class,
                                  mpc_lowcomm_communicator_t comm, int subtype,
                                  int param, void *buffer, size_t size,
                                  int rail_id)
{
	mpc_lowcomm_communicator_t communicator = comm;
	int tag = 16000;

	mpc_lowcomm_request_t request;

	mpc_lowcomm_ptp_message_t msg;

	/* Add a dummy payload */
	static int dummy_data = 42;

	if(!buffer && !size)
	{
		buffer = &dummy_data;
		size   = sizeof(int);
	}

	if(!_mpc_comm_ptp_message_is_for_control(message_class) )
	{
		mpc_common_debug_fatal("Cannot send a non-contol message using this function");
	}

	int source = -1;

	if(dest < 0)
	{
		int cw_rank = mpc_lowcomm_communicator_world_rank_of(comm, dest_task);
		dest   = mpc_lowcomm_group_process_rank_from_world(cw_rank);
		source = mpc_lowcomm_communicator_rank_of(communicator, mpc_common_get_task_rank() );
	}
	else
	{
		source = mpc_common_get_process_rank();
	}

	/* Are we sending to our own process ?? */
	//~ if( dest == mpc_common_get_process_rank() )
	//~ {
	//~ /* If so we call directly */
	//~ control_message_submit( message_class, rail_id, mpc_common_get_process_rank(),
	// mpc_lowcomm_communicator_rank_of(communicator, mpc_common_get_task_rank() ), subtype, param, buffer,
	// size );
	//~ return;
	//~ }

	mpc_lowcomm_ptp_message_header_clear(&msg, MPC_LOWCOMM_MESSAGE_CONTIGUOUS, sctk_free_control_messages,
	                                     mpc_lowcomm_ptp_message_copy);

	/* Fill in control message context (note that class is handled by
	 * set_header_in_message) */
	SCTK_MSG_SPECIFIC_CLASS_SET_SUBTYPE( (&msg), subtype);
	SCTK_MSG_SPECIFIC_CLASS_SET_PARAM( (&msg), param);
	SCTK_MSG_SPECIFIC_CLASS_SET_RAILID( (&msg), rail_id);

	mpc_lowcomm_ptp_message_set_contiguous_addr(&msg, buffer, size);

	// printpayload( buffer, size );
	mpc_lowcomm_ptp_message_header_init(&msg, tag, communicator, source,
	                                    (0 <= dest_task) ? dest_task : dest, &request,
	                                    size, message_class, MPC_DATATYPE_IGNORE, REQUEST_SEND);

	_mpc_comm_ptp_message_send_check(&msg, 1);

	mpc_lowcomm_request_wait(&request);
}

void sctk_control_messages_send_process(int dest_process, int subtype,
                                        char param, void *buffer, size_t size)
{
	__sctk_control_messages_send(dest_process, -1, MPC_LOWCOMM_CONTROL_MESSAGE_PROCESS,
	                             MPC_COMM_WORLD, subtype, param, buffer, size,
	                             -1);
}

void sctk_control_messages_send_to_task(int dest_task, mpc_lowcomm_communicator_t comm,
                                        int subtype, char param, void *buffer,
                                        size_t size)
{
	mpc_common_debug("Send task to %d (subtype %d)", dest_task, subtype);
	__sctk_control_messages_send(-1, dest_task, MPC_LOWCOMM_CONTROL_MESSAGE_TASK, comm,
	                             subtype, param, buffer, size, -1);
}

void sctk_control_messages_send_rail(int dest, int subtype, char param, void *buffer, size_t size, int rail_id)
{
	__sctk_control_messages_send(dest, -1, MPC_LOWCOMM_CONTROL_MESSAGE_RAIL,
	                             MPC_COMM_WORLD, subtype, param, buffer, size,
	                             rail_id);
}

void sctk_control_messages_incoming(mpc_lowcomm_ptp_message_t *msg)
{
	mpc_lowcomm_ptp_message_class_t class = SCTK_MSG_SPECIFIC_CLASS(msg);

	switch(class)
	{
		/* Direct processing possibly outside of MPI context */
		case MPC_LOWCOMM_CONTROL_MESSAGE_RAIL:
			sctk_control_messages_perform(msg, 0);
			break;

		/* Delayed processing but freeing the network progress threads
		 * from the handler (avoid deadlocks) */
		case MPC_LOWCOMM_CONTROL_MESSAGE_PROCESS:
			sctk_control_message_push(msg);
			break;

		default:
			not_reachable();
	}
}

void control_message_submit(mpc_lowcomm_ptp_message_class_t class, int rail_id,
                            int source_process, int source_rank, int subtype,
                            int param, void *data, size_t msg_size)
{
	switch(class)
	{
		case MPC_LOWCOMM_CONTROL_MESSAGE_RAIL: {
			sctk_rail_info_t *rail = sctk_rail_get_by_id(rail_id);

			if(!rail)
			{
				mpc_common_debug_fatal("No such rail %d", rail_id);
			}

			if(!rail->control_message_handler)
			{
				mpc_common_debug_warning("No handler was set to rail (%d = %s) control messages, set "
				                         "it prior to send such messages",
				                         rail_id, rail->network_name);
				return;
			}

			// printpayload( data, SCTK_MSG_SIZE ( msg ) );

			(rail->control_message_handler)(rail, source_process, source_rank, subtype,
			                                param, data, msg_size);
		} break;

		case MPC_LOWCOMM_CONTROL_MESSAGE_TASK: {
			sctk_control_message_task_level(source_process, source_rank, subtype, param,
			                                data, msg_size);
		} break;

		case MPC_LOWCOMM_CONTROL_MESSAGE_PROCESS: {
			sctk_control_message_process_level(source_process, source_rank, subtype,
			                                   param, data, msg_size);
		} break;

		default:
			not_reachable();
	}
}

#define SCTK_CTRL_MSG_STATICBUFFER_SIZE    2048

void sctk_control_messages_perform(mpc_lowcomm_ptp_message_t *msg, int force)
{
	mpc_lowcomm_ptp_message_class_t class = SCTK_MSG_SPECIFIC_CLASS(msg);

	if(!_mpc_comm_ptp_message_is_for_control(class) )
	{
		mpc_common_debug_fatal("Cannot process a non-control message using this function (%s)",
		                       mpc_lowcomm_ptp_message_class_name[class]);
	}

	/* Retrieve message info here as they are freed after match */
	int    source_process = SCTK_MSG_SRC_PROCESS(msg);
	int    source_rank    = SCTK_MSG_SRC_TASK(msg);
	size_t msg_size       = SCTK_MSG_SIZE(msg);
	mpc_lowcomm_communicator_t msg_comm = SCTK_MSG_COMMUNICATOR(msg);
	short subtype = SCTK_MSG_SPECIFIC_CLASS_SUBTYPE(msg);
	char  param   = SCTK_MSG_SPECIFIC_CLASS_PARAM(msg);
	char  rail_id = SCTK_MSG_SPECIFIC_CLASS_RAILID(msg);

	int   did_allocate      = 0;
	void *tmp_contol_buffer = NULL;
	char  __the_static_buffer_used_instead_of_doing_static_allocation
	[SCTK_CTRL_MSG_STATICBUFFER_SIZE];

	/* Do we need to rely on a dynamic allocation */
	if(SCTK_CTRL_MSG_STATICBUFFER_SIZE <= SCTK_MSG_SIZE(msg) )
	{
		/* Yes we will have to free */
		did_allocate = 1;
		/* Allocate */
		tmp_contol_buffer = sctk_calloc(SCTK_MSG_SIZE(msg), sizeof(char) );
	}
	else
	{
		/* No need to free */
		did_allocate = 0;
		/* Refference the static buffer */
		tmp_contol_buffer =
			__the_static_buffer_used_instead_of_doing_static_allocation;
	}

	assume(tmp_contol_buffer != NULL);

	mpc_lowcomm_request_t request;

	if( (SCTK_MSG_DEST_TASK(msg) < 0) || force)
	{
		/* Generate the paired recv message to fill the buffer in
		 * a portable way (idependently from driver) */

		mpc_lowcomm_ptp_message_t recvmsg;
		mpc_lowcomm_ptp_message_header_clear(&recvmsg, MPC_LOWCOMM_MESSAGE_CONTIGUOUS,
		                                     sctk_free_control_messages, mpc_lowcomm_ptp_message_copy);
		mpc_lowcomm_ptp_message_set_contiguous_addr(&recvmsg, tmp_contol_buffer, msg_size);
		mpc_lowcomm_ptp_message_header_init(&recvmsg, 0, msg_comm, MPC_ANY_SOURCE,
		                                    mpc_common_get_process_rank(), &request,
		                                    SCTK_MSG_SIZE(msg), class, MPC_DATATYPE_IGNORE, REQUEST_RECV);

		/* Trigger the receive task (as if we matched) */
		mpc_lowcomm_ptp_message_content_to_copy_t copy_task;
		copy_task.msg_send = msg;
		copy_task.msg_recv = &recvmsg;
		copy_task.prev     = NULL;
		copy_task.next     = NULL;
		/* Trigger the copy from network task */
		copy_task.msg_send->tail.message_copy(&copy_task);
	}
	else
	{
		mpc_lowcomm_irecv_class(source_rank, tmp_contol_buffer, msg_size, 16000,
		                        msg_comm, class, &request);
		mpc_lowcomm_request_wait(&request);
	}

	void *data = tmp_contol_buffer;

	control_message_submit(class, rail_id, source_process, source_rank, subtype,
	                       param, data, msg_size);

	/* Free only if the message was too large
	 * for the static buffer */
	if(did_allocate)
	{
		/* Free the TMP buffer */
		sctk_free(tmp_contol_buffer);
	}
}

void sctk_control_message_fence_req(int target_task, mpc_lowcomm_communicator_t comm,
                                    mpc_lowcomm_request_t *req)
{
	struct sctk_control_message_fence_ctx ctx;

	return;

	ctx.source = mpc_lowcomm_communicator_rank_of(comm, mpc_common_get_task_rank() );
	ctx.remote = mpc_lowcomm_communicator_rank_of(comm, target_task);
	ctx.comm   = comm->id;

	static int dummy = 0;

	mpc_lowcomm_irecv_class_dest(ctx.remote, ctx.source, &dummy, sizeof(int),
	                             ctx.source, comm, MPC_LOWCOMM_CONTROL_MESSAGE_FENCE,
	                             req);

	sctk_control_messages_send_process(
		mpc_lowcomm_group_process_rank_from_world(
			mpc_lowcomm_communicator_world_rank_of(comm, target_task) ),
		SCTK_PROCESS_FENCE, 0, &ctx,
		sizeof(struct sctk_control_message_fence_ctx) );
}

void sctk_control_message_fence(int target_task, mpc_lowcomm_communicator_t comm)
{
	return;

	mpc_lowcomm_request_t fence_req;

	sctk_control_message_fence_req(target_task, comm, &fence_req);

	mpc_lowcomm_request_wait(&fence_req);
}

void sctk_control_message_fence_handler(struct sctk_control_message_fence_ctx *ctx)
{
	static int dummy = 0;

	mpc_common_debug_error("FENCE SEND %d -> %d", ctx->remote, ctx->source);

	sctk_control_message_process_local(mpc_common_get_task_rank() );

	mpc_lowcomm_communicator_t comm = mpc_lowcomm_get_communicator_from_id(ctx->comm);

	mpc_lowcomm_isend_class_src(ctx->remote, ctx->source, &dummy, sizeof(int),
	                            ctx->source, comm,
	                            MPC_LOWCOMM_CONTROL_MESSAGE_FENCE, NULL);
}

/************************************************************************/
/* Control Messages List                                                */
/************************************************************************/

struct sctk_topological_polling_tree ___control_message_list_polling_tree;

struct sctk_ctrl_msg_cell
{
	mpc_lowcomm_ptp_message_t *msg;

	struct sctk_ctrl_msg_cell *prev;
	struct sctk_ctrl_msg_cell *next;
};

struct sctk_ctrl_msg_cell *__ctrl_msg_list      = NULL;
mpc_common_spinlock_t      __ctrl_msg_list_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

void sctk_control_message_push(mpc_lowcomm_ptp_message_t *msg)
{
	struct sctk_ctrl_msg_cell *cell = sctk_malloc(sizeof(struct sctk_ctrl_msg_cell) );

	cell->msg = msg;

	mpc_common_spinlock_lock(&__ctrl_msg_list_lock);
	DL_PREPEND(__ctrl_msg_list, cell);
	mpc_common_spinlock_unlock(&__ctrl_msg_list_lock);
}

void sctk_control_message_process_all()
{
	struct sctk_ctrl_msg_cell *cell = NULL;

	int present = 1;

	while(present)
	{
		if(__ctrl_msg_list)
		{
			mpc_common_spinlock_lock_yield(&__ctrl_msg_list_lock);

			if(__ctrl_msg_list)
			{
				/* In UTLIST head->prev is the tail */
				cell = __ctrl_msg_list->prev;
				DL_DELETE(__ctrl_msg_list, cell);
			}

			mpc_common_spinlock_unlock(&__ctrl_msg_list_lock);
		}

		if(cell)
		{
			sctk_control_messages_perform(cell->msg, 0);
			sctk_free(cell);
			cell = NULL;
		}
		else
		{
			present = 0;
		}
	}
}

__thread int ___is_in_sctk_control_message_process_local = 0;

int sctk_control_message_process_local(int rank)
{
	int ret = 0;

	if(rank < 0)
	{
		return ret;
	}

	if(___is_in_sctk_control_message_process_local)
	{
		return ret;
	}

	___is_in_sctk_control_message_process_local = 1;

	int st;
	mpc_lowcomm_ptp_message_header_t msg;
	memset(&msg, 0, sizeof(mpc_lowcomm_ptp_message_header_t) );

	mpc_lowcomm_message_probe_any_source_class_comm(rank, 16000, MPC_LOWCOMM_P2P_MESSAGE,
	                                                MPC_COMM_WORLD, &st, &msg);

	___is_in_sctk_control_message_process_local = 0;

	return ret;
}

void __sctk_control_message_process(__UNUSED__ void *dummy)
{
	struct sctk_ctrl_msg_cell *cell = NULL;

	if(__ctrl_msg_list != NULL)
	{
		mpc_common_spinlock_lock_yield(&__ctrl_msg_list_lock);

		if(__ctrl_msg_list != NULL)
		{
			/* In UTLIST head->prev is the tail */
			cell = __ctrl_msg_list->prev;
			DL_DELETE(__ctrl_msg_list, cell);
		}

		mpc_common_spinlock_unlock(&__ctrl_msg_list_lock);

		if(cell)
		{
			sctk_control_messages_perform(cell->msg, 0);
			sctk_free(cell);
		}
	}
}

static __thread int __inside_sctk_control_message_process = 0;

void sctk_control_message_process()
{
	__inside_sctk_control_message_process = 1;

	sctk_topological_polling_tree_poll(&___control_message_list_polling_tree,
	                                   __sctk_control_message_process, NULL);

	__inside_sctk_control_message_process = 0;
}

void sctk_control_message_init()
{
	sctk_topological_polling_tree_init(&___control_message_list_polling_tree, RAIL_POLL_SOCKET, RAIL_POLL_MACHINE, 0);
}
