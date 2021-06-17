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

#include "sctk_window.h"
#include "comm.h"
#include "uthash.h"
#include <sctk_alloc.h>
#include <mpc_common_asm.h>
#include <sctk_control_messages.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_types.h>

#include <group.h>
#include <communicator.h>

/*********************************************
* TRAMPOLINE FUNCTION FOR MPI ALLOCMEM POOL *
*********************************************/

static int (*___is_in_allocmem_pool_trampoline)(void *ptr) = NULL;

void mpc_lowcomm_rdma_allocmem_is_in_pool_set_trampoline(int (*trampoline)(void *ptr) )
{
	___is_in_allocmem_pool_trampoline = trampoline;
}

static inline int mpc_lowcomm_rdma_allocmem_is_in_pool(void *ptr)
{
	if(___is_in_allocmem_pool_trampoline)
	{
		return (___is_in_allocmem_pool_trampoline)(ptr);
	}

	return 0;
}

/************************************************************************/
/* Window Numbering and translation                                     */
/************************************************************************/

OPA_int_t __rma_generation;

void sctk_win_translation_init(struct sctk_win_translation *wt,
                               struct mpc_lowcomm_rdma_window *win)
{
	wt->win        = win;
	wt->generation = OPA_load_int(&__rma_generation);
}

static struct mpc_common_hashtable ___window_hash_table;

OPA_int_t __current_win_id;

void mpc_lowcomm_rdma_window_init_ht()
{
	mpc_common_hashtable_init(&___window_hash_table, 512);
	OPA_store_int(&__current_win_id, 1);
}

void mpc_lowcomm_rdma_window_release_ht()
{
	void *var = NULL;

	MPC_HT_ITER(&___window_hash_table, var)

	struct mpc_lowcomm_rdma_window *win = (struct mpc_lowcomm_rdma_window *)var;

	mpc_common_debug("Unfreed window on comm %u, size %ld, disp_unit %ld", win->comm_id,
	                 win->size, win->disp_unit);

	sctk_free(win);

	MPC_HT_ITER_END

	mpc_common_hashtable_release(&___window_hash_table);
}

/* The ID to win translation HT */

static struct mpc_lowcomm_rdma_window *sctk_win_register()
{
	struct mpc_lowcomm_rdma_window *new = sctk_malloc(sizeof(struct mpc_lowcomm_rdma_window) );

	assume(new);
	memset(new, 0, sizeof(struct mpc_lowcomm_rdma_window) );

	int new_id = OPA_fetch_and_add_int(&__current_win_id, 1);

	new->id = new_id;

	mpc_common_hashtable_set(&___window_hash_table, new_id, (void *)new);

	return new;
}

__thread struct sctk_win_translation __forward_translate = { 0 };

struct mpc_lowcomm_rdma_window *__sctk_win_translate(mpc_lowcomm_rdma_window_t win_id)
{
	struct mpc_lowcomm_rdma_window *ret = NULL;

	ret = mpc_common_hashtable_get(&___window_hash_table, win_id);

	assume(win_id == ret->id);

	OPA_load_int(&__rma_generation);

	if(!__forward_translate.win)
	{
		sctk_win_translation_init(&__forward_translate, ret);
	}
	else
	{
		sctk_win_translation_init(&__forward_translate, ret);
	}

	return ret;
}

static void sctk_win_delete(struct mpc_lowcomm_rdma_window *win)
{
	if(!win)
	{
		return;
	}

	OPA_incr_int(&__rma_generation);

	int id = win->id;

	mpc_common_hashtable_delete(&___window_hash_table, id);

	sctk_free(win->incoming_emulated_rma);
	memset(win, 0, sizeof(struct mpc_lowcomm_rdma_window) );
	sctk_free(win);
}

/************************************************************************/
/* Definition of an RDMA window                                         */
/************************************************************************/

static void _sctk_win_acquire(struct mpc_lowcomm_rdma_window *win)
{
	mpc_common_spinlock_lock(&win->lock);

	win->refcounter++;

	mpc_common_spinlock_unlock(&win->lock);
}

void sctk_win_acquire(mpc_lowcomm_rdma_window_t win_id)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	_sctk_win_acquire(win);
}

static int _sctk_win_relax(struct mpc_lowcomm_rdma_window *win)
{
	mpc_common_nodebug("%s win_id %d on %d", __FUNCTION__, win->id, mpc_common_get_task_rank() );
	int ret = 0;

	mpc_common_spinlock_lock(&win->lock);

	win->refcounter--;
	ret = win->refcounter;

	mpc_common_spinlock_unlock(&win->lock);

	return ret;
}

int sctk_win_relax(mpc_lowcomm_rdma_window_t win_id)
{
	mpc_common_debug_error("%s", __FUNCTION__);
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	return _sctk_win_relax(win);
}

mpc_lowcomm_rdma_window_t mpc_lowcomm_rdma_window_init(void *addr, size_t size, size_t disp_unit, mpc_lowcomm_communicator_t comm)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_register();

	/* We are local and therefore have no remote id */
	win->remote_id = -1;

	/* Set window owner task */
	win->owner        = mpc_common_get_task_rank();
	win->communicator = comm;
	win->comm_id      = mpc_lowcomm_communicator_id(comm);
	win->comm_rank    = mpc_lowcomm_communicator_rank_of(win->communicator, win->owner);

	/* Save CTX */
	win->start_addr = addr;
	win->size       = size;
	win->disp_unit  = disp_unit;

	OPA_store_int(&win->outgoing_emulated_rma, 0);

	int comm_size = mpc_lowcomm_communicator_size(win->communicator);

	win->incoming_emulated_rma =
		sctk_calloc(comm_size, sizeof(OPA_int_t) );

	int i;
	for(i = 0; i < comm_size; i++)
	{
		OPA_store_int(&win->incoming_emulated_rma[i], 0);
	}

	sctk_rail_info_t *rdma_rail = sctk_rail_get_rdma();

	if(!rdma_rail)
	{
		/* No RDMA rail flag the window as emulated */
		win->is_emulated = 1;
	}
	else
	{
		/* We have a RDMA rail no need to emulate */
		win->is_emulated = 0;
		/* We need to pin the window if it is not empty */
		if(size != 0)
		{
			sctk_rail_pin_ctx_init(&win->pin, addr, size);
		}
	}

	/* Set refcounter */
	mpc_common_spinlock_init(&win->lock, 0);
	win->refcounter = 1;

	win->access_mode = SCTK_WIN_ACCESS_AUTO;

	mpc_common_nodebug("CREATING WIN (%p) %d on %d RC %d", win, win->id,
	                   mpc_common_get_task_rank(), win->refcounter);

	return win->id;
}

void mpc_lowcomm_rdma_window_release(mpc_lowcomm_rdma_window_t win_id)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	mpc_common_nodebug("%s %p win_id %d on %d RC %d", __FUNCTION__, win, win_id,
	                   mpc_common_get_task_rank(), win->refcounter);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	/* Do we need to signal a remote win ? */
	if( (win->owner != mpc_common_get_task_rank() ) &&
	    mpc_lowcomm_is_remote_rank(win->owner) )
	{
		mpc_common_nodebug("Remote is %d on %d", win->remote_id, win->owner);

		/* This is a remote window sigal release
		 * to remote rank */
		assume(0 <= win->remote_id);

		/* Signal release to remote */
		sctk_control_messages_send_process(
			mpc_lowcomm_group_process_rank_from_world(win->owner),
			SCTK_PROCESS_RDMA_WIN_RELAX, 0, (void *)&win->remote_id,
			sizeof(int) );
	}

	/* Now work on the local window */

	/* Decrement refcounter */
	int refcount = _sctk_win_relax(win);

	/* Did the refcounter reach "0" ? */
	if(refcount != 0)
	{
		mpc_common_nodebug("NO REL %s win_id %d on %d (RC %d)", __FUNCTION__,
		                   win_id, mpc_common_get_task_rank(), refcount);
		return;
	}

	mpc_common_nodebug("REL %s win_id %d on %d (RC %d)", __FUNCTION__, win_id,
	                   mpc_common_get_task_rank(), refcount);

	/* If we are at "0" we free the window */

	mpc_lowcomm_rdma_window_local_release(win_id);
}

void mpc_lowcomm_rdma_window_local_release(mpc_lowcomm_rdma_window_t win_id)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	/* Make sure that the window is not emulated and
	 * that it is not a remote one (pinned remotely) */
	if( (!win->is_emulated) && (win->remote_id < 0) )
	{
		/* If we were not emulated, unpin */
		if(win->size != 0)
		{
			sctk_rail_pin_ctx_release(&win->pin);
		}
	}

	/* Free content */
	sctk_win_delete(win);
}

int mpc_lowcomm_rdma_window_build_from_remote(struct mpc_lowcomm_rdma_window *remote_win_data)
{
	if(remote_win_data->id < 0)
	{
		/* No such remote */
		mpc_common_debug_error("NO SUCH remote");
		return -1;
	}

	/* If we are here we received the remote and it existed */

	/* Create a new window */
	struct mpc_lowcomm_rdma_window *new_win = sctk_win_register();

	int local_id  = new_win->id;
	int remote_id = remote_win_data->id;

	mpc_common_nodebug("CREATE win %d by copying %p to %p", local_id, remote_win_data,
	                   new_win);

	/* Copy remote inside it */
	memcpy(new_win, remote_win_data, sizeof(struct mpc_lowcomm_rdma_window) );

	/* Update refcounting (1 as it is a window mirror) */
	new_win->refcounter = 1;
	mpc_common_spinlock_init(&new_win->lock, 0);

	/* Restore local ID overwritten by the copy and save remote id */
	new_win->remote_id = remote_id;

	assume(0 <= new_win->remote_id);

	new_win->id = local_id;

	new_win->payload = NULL;

	/* Warning we have a remote pointer here,
	 * this is why we reallocate the counter array */
	int comm_size = mpc_lowcomm_communicator_size(new_win->communicator);

	new_win->incoming_emulated_rma =
		sctk_calloc(comm_size, sizeof(OPA_int_t) );

	int i;
	for(i = 0; i < comm_size; i++)
	{
		OPA_store_int(&new_win->incoming_emulated_rma[i], 0);
	}

	return new_win->id;
}

int mpc_lowcomm_rdma_window_map_remote(int remote_rank, mpc_lowcomm_communicator_t comm,
                                       mpc_lowcomm_rdma_window_t win_id)
{
	mpc_common_nodebug("%s winid is %d", __FUNCTION__, win_id);

	struct mpc_lowcomm_rdma_window_map_request mr;
	mpc_lowcomm_rdma_window_map_request_init(&mr, remote_rank, win_id);

	struct mpc_lowcomm_rdma_window remote_win_data;
	memset(&remote_win_data, 0, sizeof(struct mpc_lowcomm_rdma_window) );

	/* Case where we are local */
	if( (mr.source_rank == mr.remote_rank) ||
	    (!mpc_lowcomm_is_remote_rank(mpc_lowcomm_communicator_world_rank_of(
						 comm, mr.remote_rank) ) ) ) /* If the target is in the same proces
	                                                                      * just work locally */
	{
		struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

		if(!win)
		{
			/* No such window */
			mpc_common_debug_error("No such win");
			return -1;
		}

		memcpy(&remote_win_data, win, sizeof(struct mpc_lowcomm_rdma_window) );
	}
	else
	{
		/* And now prepare to receive the remote win data */
		mpc_lowcomm_request_t req;
		mpc_lowcomm_irecv_class(remote_rank, &remote_win_data,
		                        sizeof(struct mpc_lowcomm_rdma_window), TAG_RDMA_MAP, comm,
		                        MPC_LOWCOMM_RDMA_WINDOW_MESSAGES, &req);

		/* Send a map request to remote task */
		int cw_rank = mpc_lowcomm_communicator_world_rank_of(comm, remote_rank);

		sctk_control_messages_send_process(
			mpc_lowcomm_group_process_rank_from_world(cw_rank),
			SCTK_PROCESS_RDMA_WIN_MAPTO, 0, &mr,
			sizeof(struct mpc_lowcomm_rdma_window_map_request) );

		mpc_lowcomm_request_wait(&req);
	}

	/* Make sure to reinit the communicator */
	remote_win_data.communicator = mpc_lowcomm_get_communicator_from_id(remote_win_data.comm_id);
	assume(remote_win_data.communicator != NULL);

	return mpc_lowcomm_rdma_window_build_from_remote(&remote_win_data);
}

/* Control messages handler */

void mpc_lowcomm_rdma_window_map_remote_ctrl_msg_handler(struct mpc_lowcomm_rdma_window_map_request *mr)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(mr->win_id);

	/* Note that here we are called outside of a task context
	 * therefore we have to manually retrieve the remote task
	 * id for the map request to fill the messages accordingly
	 * using mpc_lowcomm_isend_class_src */

	mpc_lowcomm_request_t req;

	if(!win)
	{
		/* We did not find the requested win
		 * send a dummy window with a negative ID*/
		struct mpc_lowcomm_rdma_window dummy_win;
		memset(&dummy_win, 0, sizeof(struct mpc_lowcomm_rdma_window) );
		dummy_win.id = -1;

		/* Send the dummy win */
		mpc_lowcomm_isend_class_src(mr->remote_rank, mr->source_rank,
		                            &dummy_win, sizeof(struct mpc_lowcomm_rdma_window),
		                            TAG_RDMA_MAP, win->communicator,
		                            MPC_LOWCOMM_RDMA_WINDOW_MESSAGES, &req);
		// mpc_lowcomm_request_wait ( &req );
		/* Done */
		return;
	}

	mpc_common_nodebug("MAP REMOTE HANDLER RESPONDS TO %d", mr->source_rank);

	/* We have a local window matching */

	/* Increment its refcounter */
	_sctk_win_acquire(win);

	/* Send local win infos to remote */

	mpc_lowcomm_isend_class_src(
		mr->remote_rank, mr->source_rank, win, sizeof(struct mpc_lowcomm_rdma_window),
		TAG_RDMA_MAP, win->communicator, MPC_LOWCOMM_RDMA_WINDOW_MESSAGES, &req);
	/* DONE */

	mpc_lowcomm_request_wait(&req);
}

void mpc_lowcomm_rdma_window_relax_ctrl_msg_handler(mpc_lowcomm_rdma_window_t win_id)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	mpc_common_nodebug("%s win is %p target is %d (RC %d)", __FUNCTION__, win, win_id, win->refcounter);

	if(!win)
	{
		mpc_common_debug_warning("No such window %d in remote relax", win_id);
		return;
	}

	mpc_lowcomm_rdma_window_release(win_id);
}

/************************************************************************/
/* Window Operations                                                    */
/************************************************************************/

void mpc_lowcomm_rdma_window_complete_request(mpc_lowcomm_request_t *req)
{
	req->completion_flag = 1;

	mpc_lowcomm_notify_request_completion(req);


	if(req->pointer_to_source_request)
	{
		mpc_lowcomm_request_t *preq = (mpc_lowcomm_request_t *)req->pointer_to_source_request;
		preq->completion_flag = 1;
	}
}

/* WRITE ======================= */

static void (*___MPC_MPI_notify_src_ctx_trampoline)(mpc_lowcomm_rdma_window_t win) = NULL;

void mpc_lowcomm_rdma_MPC_MPI_notify_src_ctx_set_trampoline(void (*trampoline)(mpc_lowcomm_rdma_window_t) )
{
	___MPC_MPI_notify_src_ctx_trampoline = trampoline;
}

static inline void __MPC_MPI_notify_src_ctx(mpc_lowcomm_rdma_window_t win)
{
	if(___MPC_MPI_notify_src_ctx_trampoline)
	{
		(___MPC_MPI_notify_src_ctx_trampoline)(win);
	}
}

static void (*___MPC_MPI_notify_dest_ctx_trampoline)(mpc_lowcomm_rdma_window_t win) = NULL;

void mpc_lowcomm_rdma_MPC_MPI_notify_dest_ctx_set_trampoline(void (*trampoline)(mpc_lowcomm_rdma_window_t) )
{
	___MPC_MPI_notify_dest_ctx_trampoline = trampoline;
}

static inline void __MPC_MPI_notify_dest_ctx(mpc_lowcomm_rdma_window_t win)
{
	if(___MPC_MPI_notify_dest_ctx_trampoline)
	{
		(___MPC_MPI_notify_dest_ctx_trampoline)(win);
	}
}

void mpc_lowcomm_rdma_window_RDMA_emulated_write_ctrl_msg_handler(
	struct mpc_lowcomm_rdma_window_emulated_RDMA *erma)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(erma->win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window in emulated RDMA write");
	}

	size_t offset = erma->offset * win->disp_unit;

	if( (win->size < (offset + erma->size) ) &&
	    (win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA emulated write operation overflows the window");
	}

	int remote_rank = mpc_lowcomm_communicator_rank_of(win->communicator, erma->source_rank);

	mpc_lowcomm_request_t req[2];
	mpc_lowcomm_irecv_class_dest(remote_rank,
	                             win->comm_rank, win->start_addr + offset,
	                             erma->size, TAG_RDMA_WRITE, win->communicator,
	                             MPC_LOWCOMM_RDMA_WINDOW_MESSAGES, &req[0]);


	int dummy;
	mpc_lowcomm_isend_class_src(
		win->comm_rank, remote_rank,
		&dummy, sizeof(int), TAG_RDMA_WRITE_ACK, win->communicator,
		MPC_LOWCOMM_RDMA_WINDOW_MESSAGES, &req[1]);
	mpc_lowcomm_waitall(2, req, SCTK_STATUS_NULL);

	OPA_incr_int(&win->incoming_emulated_rma[erma->source_rank]);

	__MPC_MPI_notify_dest_ctx(erma->win_id);
}

static inline void mpc_lowcomm_rdma_window_RDMA_write_local(struct mpc_lowcomm_rdma_window *win,
                                                            void *src_addr, size_t size,
                                                            size_t dest_offset)
{
	size_t offset = dest_offset * win->disp_unit;

	if( (win->size < (offset + size) ) &&
	    (win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA write operation overflows the window");
	}

	/* Do the local RDMA */
	memcpy(win->start_addr + offset, src_addr, size);
}

static inline void mpc_lowcomm_rdma_window_RDMA_write_net(struct mpc_lowcomm_rdma_window *win,
                                                          sctk_rail_pin_ctx_t *src_pin,
                                                          void *src_addr, size_t size,
                                                          size_t dest_offset,
                                                          mpc_lowcomm_request_t *req)
{
	sctk_rail_info_t *rdma_rail = sctk_rail_get_rdma();

	if(!rdma_rail->rdma_write)
	{
		mpc_common_debug_fatal("This rails despite flagged RDMA did not define rdma_write");
	}

	void *dest_address = win->start_addr + win->disp_unit * dest_offset;
	void *win_end_addr = win->start_addr + win->size;

	if(win_end_addr < (dest_address + size) )
	{
		mpc_common_debug_fatal("Error RDMA write operation overflows the window");
	}

	mpc_common_nodebug("RDMA WRITE NET");

	mpc_lowcomm_ptp_message_t *msg =
		mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);
	mpc_lowcomm_ptp_message_header_init(
		msg, -8, win->communicator, mpc_lowcomm_communicator_rank_of(win->communicator, mpc_common_get_task_rank() ),
		win->comm_rank, req, size, MPC_LOWCOMM_RDMA_MESSAGE, MPC_DATATYPE_IGNORE, REQUEST_RDMA);

	/* Pin local segment */
	if(!src_pin)
	{
		src_pin = sctk_malloc(sizeof(sctk_rail_pin_ctx_t) );
		assume(src_pin != NULL);
		sctk_rail_pin_ctx_init(src_pin, src_addr, size);
		req->ptr_to_pin_ctx = (void *)src_pin;
	}

	rdma_rail->rdma_write(rdma_rail, msg, src_addr, src_pin->list, dest_address,
	                      win->pin.list, size);
}

static inline void __mpc_lowcomm_rdma_window_RDMA_write(mpc_lowcomm_rdma_window_t win_id,
                                                        sctk_rail_pin_ctx_t *src_pin,
                                                        void *src_addr, size_t size,
                                                        size_t dest_offset,
                                                        mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	mpc_lowcomm_request_init(req, win->communicator, REQUEST_RDMA);

	mpc_common_nodebug("RDMA WRITE");

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	int my_rank = mpc_common_get_task_rank();

	if( (my_rank == win->owner) ||                      /* Same rank */
	    (mpc_lowcomm_rdma_allocmem_is_in_pool(win->start_addr) ) ||
	    (win->access_mode == SCTK_WIN_ACCESS_DIRECT) || /* Forced direct mode */
	    (!mpc_lowcomm_is_remote_rank(win->owner) ) /* Same process */)
	{
		/* Shared Memory */
		mpc_lowcomm_rdma_window_RDMA_write_local(win, src_addr, size, dest_offset);
		mpc_lowcomm_rdma_window_complete_request(req);
		/* Done */
		return;
	}
	else if( (win->is_emulated) ||
	         (win->access_mode == SCTK_WIN_ACCESS_EMULATED) )
	{
		/* Emulated write using control messages */
		struct mpc_lowcomm_rdma_window_emulated_RDMA erma;
		mpc_lowcomm_rdma_window_emulated_RDMA_init(&erma, win->owner, dest_offset, size,
		                                           win->remote_id);

		mpc_lowcomm_request_t data_req;
		mpc_lowcomm_isend_class(win->comm_rank, src_addr, size, TAG_RDMA_WRITE,
		                        win->communicator, MPC_LOWCOMM_RDMA_WINDOW_MESSAGES, &data_req);

		mpc_common_nodebug("WRITE to rank %d (%d) in %d", win->comm_rank, win->owner,
		                   win->communicator);
		sctk_control_messages_send_process(
			mpc_lowcomm_group_process_rank_from_world(win->owner),
			SCTK_PROCESS_RDMA_EMULATED_WRITE, 0, &erma,
			sizeof(struct mpc_lowcomm_rdma_window_emulated_RDMA) );

		__MPC_MPI_notify_src_ctx(win->id);

		mpc_lowcomm_request_t response_req;
		int dummy;
		mpc_lowcomm_irecv_class(
			win->comm_rank,
			&dummy, sizeof(int), TAG_RDMA_WRITE_ACK, win->communicator,
			MPC_LOWCOMM_RDMA_WINDOW_MESSAGES, &response_req);

		mpc_lowcomm_request_wait(&data_req);
		mpc_lowcomm_request_wait(&response_req);


		OPA_incr_int(&win->outgoing_emulated_rma);

		mpc_lowcomm_rdma_window_complete_request(req);
	}
	else
	{
		mpc_lowcomm_rdma_window_RDMA_write_net(win, src_pin, src_addr, size, dest_offset, req);
	}
}

void mpc_lowcomm_rdma_window_RDMA_write(mpc_lowcomm_rdma_window_t win_id, void *src_addr, size_t size, size_t dest_offset, mpc_lowcomm_request_t *req)
{
	__mpc_lowcomm_rdma_window_RDMA_write(win_id, NULL, src_addr, size, dest_offset, req);
}

void mpc_lowcomm_rdma_window_RDMA_write_win(mpc_lowcomm_rdma_window_t src_win_id, size_t src_offset, size_t size, mpc_lowcomm_rdma_window_t dest_win_id, size_t dest_offset, mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *src_win = sctk_win_translate(src_win_id);

	if(!src_win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	void *src_addr     = src_win->start_addr + src_offset * src_win->disp_unit;
	void *win_end_addr = src_win->start_addr + src_win->size;

	if( (win_end_addr < (src_addr + size) ) &&
	    (src_win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA write operation overflows the window");
	}

	__mpc_lowcomm_rdma_window_RDMA_write(dest_win_id, &src_win->pin, src_addr, size,
	                                     dest_offset, req);
}

/* READ ======================= */

void mpc_lowcomm_rdma_window_RDMA_emulated_read_ctrl_msg_handler(struct mpc_lowcomm_rdma_window_emulated_RDMA *erma)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(erma->win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window in emulated RDMA write");
	}

	size_t offset = erma->offset * win->disp_unit;

	if( (win->size < (offset + erma->size) ) &&
	    (win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA emulated read operation overflows the window\n"
		                       " WIN S : %ld , offset %ld, disp %ld, actual off %ld",
		                       win->size, erma->offset, win->disp_unit, offset);
	}

	mpc_lowcomm_request_t req;
	mpc_lowcomm_isend_class_src(
		win->comm_rank, mpc_lowcomm_communicator_rank_of(win->communicator, erma->source_rank),
		win->start_addr + offset, erma->size, TAG_RDMA_READ, win->communicator,
		MPC_LOWCOMM_RDMA_WINDOW_MESSAGES, &req);
	mpc_lowcomm_request_wait(&req);
}

void mpc_lowcomm_rdma_window_RDMA_read_local(struct mpc_lowcomm_rdma_window *win, void *dest_addr, size_t size, size_t src_offset)
{
	size_t offset = src_offset * win->disp_unit;

	if( (win->size < (offset + size) ) &&
	    (win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA read operation overflows the window");
	}

	/* Do the local RDMA */
	memcpy(dest_addr, win->start_addr + offset, size);
}

void mpc_lowcomm_rdma_window_RDMA_read_net(struct mpc_lowcomm_rdma_window *win, sctk_rail_pin_ctx_t *dest_pin, void *dest_addr, size_t size, size_t src_offset, mpc_lowcomm_request_t *req)
{
	sctk_rail_info_t *rdma_rail = sctk_rail_get_rdma();

	if(!rdma_rail->rdma_write)
	{
		mpc_common_debug_fatal("This rails despite flagged RDMA did not define rdma_write");
	}

	void *src_address = win->start_addr + win->disp_unit * src_offset;

	size_t offset = src_offset * win->disp_unit;

	if(win->size < (offset + size) )
	{
		mpc_common_debug_fatal("Error RDMA read operation overflows the window");
	}

	mpc_lowcomm_ptp_message_t *msg =
		mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);

	mpc_lowcomm_ptp_message_header_init(
		msg, -8, win->communicator, mpc_lowcomm_communicator_rank_of(win->communicator, mpc_common_get_task_rank() ),
		win->comm_rank, req, size, MPC_LOWCOMM_RDMA_MESSAGE, MPC_DATATYPE_IGNORE, REQUEST_RDMA);

	/* Pin local segment */
	if(!dest_pin)
	{
		dest_pin = sctk_malloc(sizeof(sctk_rail_pin_ctx_t) );
		assume(dest_pin != NULL);
		sctk_rail_pin_ctx_init(dest_pin, dest_addr, size);
		req->ptr_to_pin_ctx = (void *)dest_pin;
	}

	rdma_rail->rdma_read(rdma_rail, msg, src_address, win->pin.list, dest_addr,
	                     dest_pin->list, size);
}

void __mpc_lowcomm_rdma_window_RDMA_read(mpc_lowcomm_rdma_window_t win_id, sctk_rail_pin_ctx_t *dest_pin, void *dest_addr, size_t size, size_t src_offset, mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	mpc_lowcomm_request_init(req, win->communicator, REQUEST_RDMA);

	mpc_common_nodebug("RDMA READ");

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID %d", win_id);
	}

	/* Set an empty request */

	int my_rank = mpc_common_get_task_rank();

	if( (my_rank == win->owner) || /* Same rank */
	    (mpc_lowcomm_rdma_allocmem_is_in_pool(win->start_addr) ) ||
	    (win->access_mode ==
	     SCTK_WIN_ACCESS_DIRECT) || /* Forced direct mode */
	    (!mpc_lowcomm_is_remote_rank(win->owner) ) /* Same process */)
	{
		/* Shared Memory */
		mpc_lowcomm_rdma_window_RDMA_read_local(win, dest_addr, size, src_offset);
		mpc_lowcomm_rdma_window_complete_request(req);
		return;
	}
	else if( (win->is_emulated) ||
	         (win->access_mode == SCTK_WIN_ACCESS_EMULATED) )
	{
		/* Emulated write using control messages */
		struct mpc_lowcomm_rdma_window_emulated_RDMA erma;
		mpc_lowcomm_rdma_window_emulated_RDMA_init(&erma, win->owner, src_offset, size,
		                                           win->remote_id);
		/* Note that we store the data transfer req in the request */
		mpc_lowcomm_irecv_class(win->comm_rank, dest_addr, size,
		                        TAG_RDMA_READ, win->communicator,
		                        MPC_LOWCOMM_RDMA_WINDOW_MESSAGES, req);

		sctk_control_messages_send_process(
			mpc_lowcomm_group_process_rank_from_world(win->owner),
			SCTK_PROCESS_RDMA_EMULATED_READ, 0, &erma,
			sizeof(struct mpc_lowcomm_rdma_window_emulated_RDMA) );
	}
	else
	{
		mpc_lowcomm_rdma_window_RDMA_read_net(win, dest_pin, dest_addr, size, src_offset,
		                                      req);
	}
}

void mpc_lowcomm_rdma_window_RDMA_read(mpc_lowcomm_rdma_window_t win_id, void *dest_addr, size_t size, size_t src_offset, mpc_lowcomm_request_t *req)
{
	__mpc_lowcomm_rdma_window_RDMA_read(win_id, NULL, dest_addr, size, src_offset, req);
}

void mpc_lowcomm_rdma_window_RDMA_read_win(mpc_lowcomm_rdma_window_t src_win_id, size_t src_offset, size_t size, mpc_lowcomm_rdma_window_t dest_win_id, size_t dest_offset, mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *dest_win = sctk_win_translate(dest_win_id);

	if(!dest_win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	void *dest_addr =
		dest_win->start_addr + dest_offset * dest_win->disp_unit;
	void *win_end_addr = dest_win->start_addr + dest_win->size;

	if( (win_end_addr < (dest_addr + size) ) &&
	    (dest_win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA read operation overflows the window");
	}

	__mpc_lowcomm_rdma_window_RDMA_read(src_win_id, &dest_win->pin, dest_addr, size,
	                                    src_offset, req);
}

/* Fetch and Add =================== */


size_t RDMA_type_size(RDMA_type type)
{
	switch(type)
	{
		case RDMA_TYPE_CHAR:
			return sizeof(char);

		case RDMA_TYPE_DOUBLE:
			return sizeof(double);

		case RDMA_TYPE_FLOAT:
			return sizeof(float);

		case RDMA_TYPE_INT:
			return sizeof(int);

		case RDMA_TYPE_LONG:
			return sizeof(long);

		case RDMA_TYPE_LONG_DOUBLE:
			return sizeof(long double);

		case RDMA_TYPE_LONG_LONG:
			return sizeof(long long);

		case RDMA_TYPE_LONG_LONG_INT:
			return sizeof(long long int);

		case RDMA_TYPE_SHORT:
			return sizeof(short);

		case RDMA_TYPE_SIGNED_CHAR:
			return sizeof(signed char);

		case RDMA_TYPE_UNSIGNED:
			return sizeof(unsigned);

		case RDMA_TYPE_UNSIGNED_CHAR:
			return sizeof(unsigned char);

		case RDMA_TYPE_UNSIGNED_LONG:
			return sizeof(unsigned long);

		case RDMA_TYPE_UNSIGNED_LONG_LONG:
			return sizeof(unsigned long long);

		case RDMA_TYPE_UNSIGNED_SHORT:
			return sizeof(unsigned short);

		case RDMA_TYPE_WCHAR:
			return sizeof(sctk_wchar_t);

		case RDMA_TYPE_AINT:
			return sizeof(void *);

		case RDMA_TYPE_NULL:
			not_reachable();
			break;
	}

	mpc_common_debug_fatal("Found an unhandled RDMA datatype");
	return -1;
}

static inline void mpc_lowcomm_rdma_window_fetch_and_op_operate_int(RDMA_op op, void *add,
                                                                    void *src, void *dest)
{
	int result = 0;

	int val = 0;

	int *to_add = ( (int *)add);

	switch(op)
	{
		case RDMA_SUM:
			result = OPA_fetch_and_add_int(src, *to_add);
			break;

		case RDMA_INC:
			result = OPA_fetch_and_incr_int(src);
			break;

		case RDMA_DEC:
			result = OPA_fetch_and_decr_int(src);
			break;

		case RDMA_MIN:
			val = OPA_load_int(src);

			if(*to_add < val)
			{
				OPA_store_int(src, *to_add);
			}

			result = val;
			break;

		case RDMA_MAX:
			val = OPA_load_int(src);

			if(val < *to_add)
			{
				OPA_store_int(src, *to_add);
			}

			result = val;
			break;

		case RDMA_PROD:
			val = OPA_load_int(src);
			OPA_store_int(src, *to_add * val);
			result = val;
			break;

		case RDMA_LAND:
			val = OPA_load_int(src);
			OPA_store_int(src, *to_add && val);
			result = val;
			break;

		case RDMA_BAND:
			val = OPA_load_int(src);
			OPA_store_int(src, *to_add & val);
			result = val;
			break;

		case RDMA_LOR:
			val = OPA_load_int(src);
			OPA_store_int(src, *to_add || val);
			result = val;
			break;

		case RDMA_BOR:
			val = OPA_load_int(src);
			OPA_store_int(src, *to_add | val);
			result = val;
			break;

		case RDMA_LXOR:
			val = OPA_load_int(src);
			OPA_store_int(src, (!(*to_add) ) != (!(val) ) );
			result = val;
			break;

		case RDMA_BXOR:
			val = OPA_load_int(src);
			OPA_store_int(src, *to_add ^ val);
			result = val;
			break;

		case RDMA_OP_NULL:
			not_reachable();
			break;
	}

	memcpy(dest, &result, sizeof(OPA_int_t) );
}

#define RDMA_OP_def(type, type2, type3)                                                                  \
	static mpc_common_spinlock_t __RDMA_OP_ ## type ## type2 ## type3 ## _lock =                     \
		SCTK_SPINLOCK_INITIALIZER;                                                               \
                                                                                                         \
	static inline void mpc_lowcomm_rdma_window_fetch_and_op_operate_ ## type ## type2 ## type3 ## _( \
		RDMA_op op, void *add, void *src, void *dest){                                           \
		type type2 type3 *to_add    = (type type2 type3 *)add;                                   \
		type type2 type3 *src_addr  = (type type2 type3 *)src;                                   \
		type type2 type3 *dest_addr = (type type2 type3 *)dest;                                  \
                                                                                                         \
		mpc_common_spinlock_lock(&__RDMA_OP_ ## type ## type2 ## type3 ## _lock);                \
                                                                                                         \
		*dest_addr = *src_addr;                                                                  \
                                                                                                         \
		switch(op){                                                                              \
			case RDMA_SUM:                                                                   \
				*src_addr = *src_addr + *to_add;                                         \
				break;                                                                   \
                                                                                                         \
			case RDMA_INC:                                                                   \
				*src_addr = *src_addr + 1;                                               \
				break;                                                                   \
                                                                                                         \
			case RDMA_DEC:                                                                   \
				*src_addr = *src_addr - 1;                                               \
				break;                                                                   \
                                                                                                         \
			case RDMA_MIN:                                                                   \
				if(*to_add < *src_addr){                                                 \
					*src_addr = *to_add;                                             \
				}                                                                        \
				break;                                                                   \
			case RDMA_MAX:                                                                   \
				if(*src_addr < *to_add){                                                 \
					*src_addr = *to_add;                                             \
				}                                                                        \
				break;                                                                   \
			case RDMA_PROD:                                                                  \
				*src_addr *= *to_add;                                                    \
				break;                                                                   \
			case RDMA_LAND:                                                                  \
				*src_addr = *src_addr && *to_add;                                        \
				break;                                                                   \
                                                                                                         \
			case RDMA_BAND:                                                                  \
				*src_addr = *src_addr & *to_add;                                         \
				break;                                                                   \
                                                                                                         \
			case RDMA_LOR:                                                                   \
				*src_addr = *src_addr || *to_add;                                        \
				break;                                                                   \
                                                                                                         \
			case RDMA_BOR:                                                                   \
				*src_addr = *src_addr | *to_add;                                         \
				break;                                                                   \
                                                                                                         \
			case RDMA_LXOR:                                                                  \
				*src_addr = (!(*src_addr) ) != (!(*to_add) );                            \
				break;                                                                   \
                                                                                                         \
			case RDMA_BXOR:                                                                  \
				*src_addr = *src_addr ^ *to_add;                                         \
				break;                                                                   \
			case RDMA_OP_NULL:                                                               \
				not_reachable();                                                         \
				break;                                                                   \
		}                                                                                        \
                                                                                                         \
		mpc_common_spinlock_unlock(&__RDMA_OP_ ## type ## type2 ## type3 ## _lock);              \
	}

#define RDMA_OP_def_nobin(type, type2)                                                                      \
	static mpc_common_spinlock_t __RDMA_OP_ ## type ## type2 ## _lock =                                 \
		SCTK_SPINLOCK_INITIALIZER;                                                                  \
                                                                                                            \
	static inline void mpc_lowcomm_rdma_window_fetch_and_op_operate_ ## type ## type2 ## _(             \
		RDMA_op op, void *add, void *src, void *dest){                                              \
		type type2 *to_add    = (type type2 *)add;                                                  \
		type type2 *src_addr  = (type type2 *)src;                                                  \
		type type2 *dest_addr = (type type2 *)dest;                                                 \
                                                                                                            \
		mpc_common_spinlock_lock(&__RDMA_OP_ ## type ## type2 ## _lock);                            \
                                                                                                            \
		*dest_addr = *src_addr;                                                                     \
                                                                                                            \
		switch(op){                                                                                 \
			case RDMA_SUM:                                                                      \
				*src_addr += *to_add;                                                       \
				break;                                                                      \
                                                                                                            \
			case RDMA_INC:                                                                      \
				*src_addr = *src_addr + 1;                                                  \
				break;                                                                      \
                                                                                                            \
			case RDMA_DEC:                                                                      \
				*src_addr = *src_addr - 1;                                                  \
				break;                                                                      \
                                                                                                            \
			case RDMA_MIN:                                                                      \
				if(*to_add < *src_addr){                                                    \
					*src_addr = *to_add;                                                \
				}                                                                           \
				break;                                                                      \
			case RDMA_MAX:                                                                      \
				if(*src_addr < *to_add){                                                    \
					*src_addr = *to_add;                                                \
				}                                                                           \
				break;                                                                      \
			case RDMA_PROD:                                                                     \
				*src_addr *= *to_add;                                                       \
				break;                                                                      \
			case RDMA_LAND:                                                                     \
				*src_addr = *src_addr && *to_add;                                           \
				break;                                                                      \
                                                                                                            \
			case RDMA_BAND:                                                                     \
				mpc_common_debug_fatal("RDMA Binary operand is not defined for %s", #type); \
				break;                                                                      \
                                                                                                            \
			case RDMA_LOR:                                                                      \
				*src_addr = *src_addr || *to_add;                                           \
				break;                                                                      \
                                                                                                            \
			case RDMA_BOR:                                                                      \
				mpc_common_debug_fatal("RDMA Binary operand is not defined for %s", #type); \
				break;                                                                      \
                                                                                                            \
			case RDMA_LXOR:                                                                     \
				*src_addr = (!(*src_addr) ) != (!(*to_add) );                               \
				break;                                                                      \
                                                                                                            \
			case RDMA_BXOR:                                                                     \
				mpc_common_debug_fatal("RDMA Binary operand is not defined for %s", #type); \
				break;                                                                      \
			case RDMA_OP_NULL:                                                                  \
				not_reachable();                                                            \
				break;                                                                      \
		}                                                                                           \
                                                                                                            \
		mpc_common_spinlock_unlock(&__RDMA_OP_ ## type ## type2 ## _lock);                          \
	}

RDMA_OP_def(char, , )
RDMA_OP_def_nobin(double, )
RDMA_OP_def_nobin(float, )
RDMA_OP_def(long, , )
RDMA_OP_def_nobin(long, double)
RDMA_OP_def(long, long, )
RDMA_OP_def(long, long, int)
RDMA_OP_def(short, , )
RDMA_OP_def(signed, char, )
RDMA_OP_def(unsigned, , )
RDMA_OP_def(unsigned, char, )
RDMA_OP_def(unsigned, long, )
RDMA_OP_def(unsigned, long, long)
RDMA_OP_def(unsigned, short, )
RDMA_OP_def(sctk_wchar_t, , )


static inline void mpc_lowcomm_rdma_window_fetch_and_op_operate(
	RDMA_op op, RDMA_type type, void *add, void *src,
	void *dest)
{
	switch(type)
	{
		case RDMA_TYPE_CHAR:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_char_(op, add, src, dest);
			return;

		case RDMA_TYPE_DOUBLE:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_double_(op, add, src, dest);
			return;

		case RDMA_TYPE_FLOAT:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_float_(op, add, src, dest);
			return;

		case RDMA_TYPE_INT:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_int(op, add, src, dest);
			return;

		case RDMA_TYPE_LONG:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_long_(op, add, src, dest);
			return;

		case RDMA_TYPE_LONG_DOUBLE:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_longdouble_(op, add, src, dest);
			return;

		case RDMA_TYPE_LONG_LONG:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_longlong_(op, add, src, dest);
			return;

		case RDMA_TYPE_LONG_LONG_INT:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_longlongint_(op, add, src, dest);
			return;

		case RDMA_TYPE_SHORT:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_short_(op, add, src, dest);
			return;

		case RDMA_TYPE_SIGNED_CHAR:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_signedchar_(op, add, src, dest);
			return;

		case RDMA_TYPE_UNSIGNED:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_unsigned_(op, add, src, dest);
			return;

		case RDMA_TYPE_UNSIGNED_CHAR:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_unsignedchar_(op, add, src, dest);
			return;

		case RDMA_TYPE_UNSIGNED_LONG:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_unsignedlong_(op, add, src, dest);
			return;

		case RDMA_TYPE_UNSIGNED_LONG_LONG:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_unsignedlonglong_(op, add, src, dest);
			return;

		case RDMA_TYPE_UNSIGNED_SHORT:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_unsignedshort_(op, add, src, dest);
			return;

		case RDMA_TYPE_WCHAR:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_sctk_wchar_t_(op, add, src, dest);
			return;

		case RDMA_TYPE_AINT:
			mpc_lowcomm_rdma_window_fetch_and_op_operate_unsignedlonglong_(op, add, src, dest);
			return;

		case RDMA_TYPE_NULL:
			not_reachable();
			break;
	}
}

void mpc_lowcomm_rdma_window_RDMA_fetch_and_op_ctrl_msg_handler(struct mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA *fop)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(fop->rdma.win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window in emulated RDMA fetch and op");
	}

	size_t offset = fop->rdma.offset * win->disp_unit;

	if( (win->size < (offset + fop->rdma.size) ) &&
	    (win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal(
			"Error RDMA emulated feth and op operation overflows the window\n"
			" WIN S : %ld , offset %ld, disp %ld, actual off %ld",
			win->size, fop->rdma.offset, win->disp_unit, offset);
	}

	char fetch[16];

	mpc_lowcomm_rdma_window_fetch_and_op_operate(fop->op, fop->type, fop->add,
	                                             win->start_addr + offset, &fetch);

	mpc_lowcomm_isend_class_src(
		win->comm_rank, mpc_lowcomm_communicator_rank_of(win->communicator, fop->rdma.source_rank),
		&fetch, fop->rdma.size, TAG_RDMA_FETCH_AND_OP, win->communicator,
		MPC_LOWCOMM_RDMA_MESSAGE, NULL);
}

static inline void mpc_lowcomm_rdma_window_RDMA_fetch_and_op_local(
	mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, void *fetch_addr,
	void *add, RDMA_op op, RDMA_type type, mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(remote_win_id);

	// mpc_common_debug_error("Fetch and add");
	mpc_lowcomm_request_init(req, win->communicator, REQUEST_RDMA);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID %d", remote_win_id);
	}

	size_t offset = remote_offset * win->disp_unit;

	if( (win->size < (offset + RDMA_type_size(type) ) ) &&
	    (win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA fetch and op operation overflows the window");
	}

	void *remote_addr = win->start_addr + offset;

	mpc_lowcomm_rdma_window_fetch_and_op_operate(op, type, add, remote_addr, fetch_addr);

	mpc_lowcomm_rdma_window_complete_request(req);
}

static inline void mpc_lowcomm_rdma_window_RDMA_fetch_and_op_net(
	mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset,
	sctk_rail_pin_ctx_t *fetch_pin, void *fetch_addr, void *add, RDMA_op op,
	RDMA_type type, mpc_lowcomm_request_t *req)
{
	sctk_rail_info_t *rdma_rail = sctk_rail_get_rdma();

	if(!rdma_rail->rdma_fetch_and_op)
	{
		mpc_common_debug_fatal("This rails despite flagged RDMA did not define fetch and op");
	}

	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(remote_win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID %d", remote_win_id);
	}

	void *src_address = win->start_addr + win->disp_unit * remote_offset;

	size_t offset = remote_offset * win->disp_unit;

	if(win->size < (offset + RDMA_type_size(type) ) )
	{
		mpc_common_debug_fatal("Error RDMA fetch and op operation overflows the window");
	}

	mpc_lowcomm_ptp_message_t *msg =
		mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);

	mpc_lowcomm_ptp_message_header_init(msg, -8, win->communicator,
	                                    mpc_lowcomm_communicator_rank_of(win->communicator, mpc_common_get_task_rank() ),
	                                    win->comm_rank, req, RDMA_type_size(type),
	                                    MPC_LOWCOMM_RDMA_MESSAGE, MPC_DATATYPE_IGNORE, REQUEST_RDMA);

	/* Pin local segment */
	if(!fetch_pin)
	{
		fetch_pin = sctk_malloc(sizeof(sctk_rail_pin_ctx_t) );
		assume(fetch_pin != NULL);
		sctk_rail_pin_ctx_init(fetch_pin, fetch_addr, RDMA_type_size(type) );
		req->ptr_to_pin_ctx = (void *)fetch_pin;
	}

	rdma_rail->rdma_fetch_and_op(rdma_rail, msg, fetch_addr, fetch_pin->list,
	                             src_address, win->pin.list, add, op, type);
}

static inline void __mpc_lowcomm_rdma_window_RDMA_fetch_and_op(
	mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset,
	sctk_rail_pin_ctx_t *fetch_pin, void *fetch_addr, void *add, RDMA_op op,
	RDMA_type type, mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(remote_win_id);

	mpc_lowcomm_request_init(req, win->communicator, REQUEST_RDMA);

	// mpc_common_debug_error("Fetch and add");

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID %d", remote_win_id);
	}

	/* Now try to see if we pass the RDMA rail gate function for fetch and op */

	sctk_rail_info_t *rdma_rail = sctk_rail_get_rdma();

	int RDMA_fetch_and_op_gate_passed = 0;

	if(rdma_rail)
	{
		if(rdma_rail->rdma_fetch_and_op_gate)
		{
			RDMA_fetch_and_op_gate_passed = (rdma_rail->rdma_fetch_and_op_gate)(
				rdma_rail, RDMA_type_size(type), op, type);

			/* At this point if RDMA_fetch_and_op_gate_passed == 1 , the NET can
			 * handle this fetch and op */
		}
	}

	/* Set an empty request */

	int my_rank = mpc_common_get_task_rank();

	if( (my_rank == win->owner) || /* Same rank */
	    (mpc_lowcomm_rdma_allocmem_is_in_pool(win->start_addr) ) ||
	    (win->access_mode == SCTK_WIN_ACCESS_DIRECT) ||
	    (!mpc_lowcomm_is_remote_rank(win->owner) ) /* Same process */)
	{
		/* Shared Memory */
		mpc_lowcomm_rdma_window_RDMA_fetch_and_op_local(remote_win_id, remote_offset,
		                                                fetch_addr, add, op, type, req);
		mpc_lowcomm_rdma_window_complete_request(req);
		return;
	}
	else if(
		(win->is_emulated) ||
		(RDMA_fetch_and_op_gate_passed ==
		 0 /* Network does not support this RDMA atomic fallback to emulated */) ||
		(win->access_mode == SCTK_WIN_ACCESS_EMULATED) )
	{
		struct mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA fop;
		mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA_init(&fop, win->owner, remote_offset,
		                                                        win->remote_id, op, type, add);

		/* Note that we store the data transfer req in the request */
		mpc_lowcomm_irecv_class(win->comm_rank, fetch_addr, fop.rdma.size,
		                        TAG_RDMA_FETCH_AND_OP, win->communicator,
		                        MPC_LOWCOMM_RDMA_MESSAGE, req);
		sctk_control_messages_send_process(
			mpc_lowcomm_group_process_rank_from_world(win->owner),
			SCTK_PROCESS_RDMA_EMULATED_FETCH_AND_OP, 0, &fop,
			sizeof(struct mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA) );
	}
	else
	{
		mpc_lowcomm_rdma_window_RDMA_fetch_and_op_net(remote_win_id, remote_offset, fetch_pin,
		                                              fetch_addr, add, op, type, req);
	}
}

void mpc_lowcomm_rdma_window_RDMA_fetch_and_op(mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, void *fetch_addr, void *add, RDMA_op op, RDMA_type type, mpc_lowcomm_request_t *req)
{
	__mpc_lowcomm_rdma_window_RDMA_fetch_and_op(remote_win_id, remote_offset, NULL, fetch_addr, add, op, type, req);
}

void mpc_lowcomm_rdma_window_RDMA_fetch_and_add_win(mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, mpc_lowcomm_rdma_window_t local_win_id, size_t fetch_offset, void *add, RDMA_op op, RDMA_type type, mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *local_win = sctk_win_translate(local_win_id);

	if(!local_win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	void *dest_addr =
		local_win->start_addr + fetch_offset * local_win->disp_unit;
	void *win_end_addr = local_win->start_addr + local_win->size;

	if( (win_end_addr < (dest_addr + RDMA_type_size(type) ) ) &&
	    (local_win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA Fetch and OP operation overflows the window");
	}

	__mpc_lowcomm_rdma_window_RDMA_fetch_and_op(remote_win_id, remote_offset,
	                                            &local_win->pin, dest_addr, add, op,
	                                            type, req);
}

/* Compare and SWAP =================== */


void mpc_lowcomm_rdma_window_RDMA_CAS_int_local(void *cmp, void *new, void *target, void *res)
{
	int        oldv    = *( (int *)cmp);
	int        newv    = *( (int *)new);
	OPA_int_t *ptarget = (OPA_int_t *)target;
	int *      pres    = (int *)res;

	int local = 0;

	local = OPA_cas_int(ptarget, oldv, newv);

	// mpc_common_debug_error("OLD %d NEW %d RES %d", oldv, newv, local );

	if(pres)
	{
		*pres = local;
	}
}

void mpc_lowcomm_rdma_window_RDMA_CAS_ptr_local(void **cmp, void **new, void *target, void *res)
{
	OPA_ptr_t *ptarget = (OPA_ptr_t *)target;

	void *local = OPA_cas_ptr(ptarget, *cmp, *new);

	if(res)
	{
		memcpy(res, &local, sizeof(void *) );
	}
}

static mpc_common_spinlock_t __RDMA_CAS_16_lock = SCTK_SPINLOCK_INITIALIZER;

void mpc_lowcomm_rdma_window_RDMA_CAS_16_local(void *cmp, void *new, void *target, void *res)
{
	mpc_common_spinlock_lock(&__RDMA_CAS_16_lock);

	if(memcmp(target, cmp, 16) == 0)
	{
		if(res)
		{
			memcpy(res, target, 16);
		}
	}

	memcpy(target, new, 16);

	mpc_common_spinlock_unlock(&__RDMA_CAS_16_lock);
}

static mpc_common_spinlock_t __RDMA_CAS_any_lock = SCTK_SPINLOCK_INITIALIZER;

void mpc_lowcomm_rdma_window_RDMA_CAS_any_local(void *cmp, void *new, void *target, void *res, size_t size)
{
	mpc_common_spinlock_lock(&__RDMA_CAS_any_lock);

	if(memcmp(target, cmp, size) == 0)
	{
		if(res)
		{
			memcpy(res, target, size);
		}
	}

	memcpy(target, new, size);

	mpc_common_spinlock_unlock(&__RDMA_CAS_any_lock);
}

void mpc_lowcomm_rdma_window_RDMA_CAS_local(mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, void *comp, void *new_data, void *res, RDMA_type type)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(remote_win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID %d", remote_win_id);
	}

	size_t offset = remote_offset * win->disp_unit;

	if( (win->size < (offset + RDMA_type_size(type) ) ) &&
	    (win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA CAS operation overflows the window");
	}

	void *remote_addr = (OPA_int_t *)(win->start_addr + offset);

	size_t type_size = RDMA_type_size(type);

	if(type_size == sizeof(OPA_int_t) )
	{
		mpc_lowcomm_rdma_window_RDMA_CAS_int_local(comp, new_data, remote_addr, res);
	}
	else if(type_size == sizeof(OPA_ptr_t) )
	{
		mpc_lowcomm_rdma_window_RDMA_CAS_ptr_local(comp, new_data, remote_addr, res);
	}
	else if(type_size == 16)
	{
		mpc_lowcomm_rdma_window_RDMA_CAS_16_local(comp, new_data, remote_addr, res);
	}
	else
	{
		mpc_lowcomm_rdma_window_RDMA_CAS_any_local(comp, new_data, remote_addr, res,
		                                           type_size);
	}
}

void mpc_lowcomm_rdma_window_RDMA_CAS_ctrl_msg_handler(struct mpc_lowcomm_rdma_window_emulated_CAS_RDMA *fcas)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(fcas->rdma.win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window in emulated RDMA CAS");
	}

	size_t offset = fcas->rdma.offset * win->disp_unit;

	if( (win->size < (offset + fcas->rdma.size) ) &&
	    (win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA emulated CAS operation overflows the window\n"
		                       " WIN S : %ld , offset %ld, disp %ld, actual off %ld",
		                       win->size, fcas->rdma.offset, win->disp_unit, offset);
	}

	char res[16];
	mpc_lowcomm_rdma_window_RDMA_CAS_local(fcas->rdma.win_id, fcas->rdma.offset,
	                                       fcas->comp, fcas->new, res, fcas->type);

	mpc_lowcomm_isend_class_src(
		win->comm_rank, mpc_lowcomm_communicator_rank_of(win->communicator, fcas->rdma.source_rank),
		&res, fcas->rdma.size, TAG_RDMA_CAS, win->communicator, MPC_LOWCOMM_RDMA_MESSAGE,
		NULL);
}

void mpc_lowcomm_rdma_window_RDMA_CAS_net(mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, void *comp, void *new_data, void *res, sctk_rail_pin_ctx_t *res_pin, RDMA_type type, mpc_lowcomm_request_t *req)
{
	sctk_rail_info_t *rdma_rail = sctk_rail_get_rdma();

	if(!rdma_rail->rdma_cas)
	{
		mpc_common_debug_fatal("This rails despite flagged RDMA did not define CAS");
	}

	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(remote_win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID %d", remote_win_id);
	}

	void *dest_addr = win->start_addr + win->disp_unit * remote_offset;

	size_t offset = remote_offset * win->disp_unit;

	if(win->size < (offset + RDMA_type_size(type) ) )
	{
		mpc_common_debug_fatal("Error RDMA CAS operation overflows the window");
	}

	mpc_lowcomm_ptp_message_t *msg =
		mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);

	mpc_lowcomm_ptp_message_header_init(msg, -8, win->communicator,
	                                    mpc_lowcomm_communicator_rank_of(win->communicator, win->owner),
	                                    win->comm_rank, req, RDMA_type_size(type),
	                                    MPC_LOWCOMM_RDMA_MESSAGE, MPC_DATATYPE_IGNORE, REQUEST_RDMA);

	/* Pin local segment */
	if(!res_pin)
	{
		res_pin = sctk_malloc(sizeof(sctk_rail_pin_ctx_t) );
		assume(res_pin != NULL);
		sctk_rail_pin_ctx_init(res_pin, res, RDMA_type_size(type) );
		req->ptr_to_pin_ctx = (void *)res_pin;
	}

	rdma_rail->rdma_cas(rdma_rail, msg, res, res_pin->list, dest_addr,
	                    win->pin.list, comp, new_data, type);
}

void __mpc_lowcomm_rdma_window_RDMA_CAS(mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, void *comp, void *new_data, void *res, sctk_rail_pin_ctx_t *res_pin, RDMA_type type, mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(remote_win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID %d", remote_win_id);
	}

	mpc_lowcomm_request_init(req, win->communicator, REQUEST_RDMA);

	/* Now try to see if we pass the RDMA rail gate function for fetch and
	 * op */

	sctk_rail_info_t *rdma_rail = sctk_rail_get_rdma();

	int RDMA_CAS_gate_passed = 0;

	if(rdma_rail)
	{
		if(rdma_rail->rdma_cas_gate)
		{
			RDMA_CAS_gate_passed = (rdma_rail->rdma_cas_gate)(
				rdma_rail, RDMA_type_size(type), type);

			/* At this point if RDMA_CAS_gate_passed == 1 , the NET can handle
			 * CAS */
		}
	}

	/* Set an empty request */

	int my_rank = mpc_common_get_task_rank();

	if( (my_rank == win->owner) || /* Same rank */
	    (mpc_lowcomm_rdma_allocmem_is_in_pool(win->start_addr) ) ||
	    (win->access_mode == SCTK_WIN_ACCESS_DIRECT) ||
	    (!mpc_lowcomm_is_remote_rank(win->owner) ) /* Same process */)
	{
		/* Shared Memory */
		mpc_lowcomm_rdma_window_RDMA_CAS_local(remote_win_id, remote_offset, comp,
		                                       new_data, res, type);
		mpc_lowcomm_rdma_window_complete_request(req);
		return;
	}
	else if(
		(win->is_emulated) ||
		(RDMA_CAS_gate_passed ==
		 0 /* Network does not support this RDMA atomic fallback to emulated */) ||
		(win->access_mode == SCTK_WIN_ACCESS_EMULATED) )
	{
		struct mpc_lowcomm_rdma_window_emulated_CAS_RDMA fcas;
		mpc_lowcomm_rdma_window_emulated_CAS_RDMA_init(&fcas, win->owner, remote_offset,
		                                               win->remote_id, type, comp,
		                                               new_data);

		mpc_lowcomm_irecv_class(win->comm_rank, res, fcas.rdma.size,
		                        TAG_RDMA_CAS, win->communicator, MPC_LOWCOMM_RDMA_MESSAGE,
		                        req);
		sctk_control_messages_send_process(
			mpc_lowcomm_group_process_rank_from_world(win->owner),
			SCTK_PROCESS_RDMA_EMULATED_CAS, 0, &fcas,
			sizeof(struct mpc_lowcomm_rdma_window_emulated_CAS_RDMA) );
	}
	else
	{
		mpc_lowcomm_rdma_window_RDMA_CAS_net(remote_win_id, remote_offset, comp, new_data,
		                                     res, res_pin, type, req);
	}
}

void mpc_lowcomm_rdma_window_RDMA_CAS(mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, void *comp, void *new_data, void *res, RDMA_type type, mpc_lowcomm_request_t *req)
{
	__mpc_lowcomm_rdma_window_RDMA_CAS(remote_win_id, remote_offset, comp, new_data, res, NULL, type, req);
}

void mpc_lowcomm_rdma_window_RDMA_CAS_win(mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, mpc_lowcomm_rdma_window_t local_win_id, size_t res_offset, void *comp, void *new_data, RDMA_type type, mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *local_win = sctk_win_translate(local_win_id);

	if(!local_win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	void *res_addr =
		local_win->start_addr + res_offset * local_win->disp_unit;
	void *win_end_addr = local_win->start_addr + local_win->size;

	if( (win_end_addr < (res_addr + RDMA_type_size(type) ) ) &&
	    (local_win->access_mode != SCTK_WIN_ACCESS_EMULATED) )
	{
		mpc_common_debug_fatal("Error RDMA CAS operation overflows the window");
	}

	__mpc_lowcomm_rdma_window_RDMA_CAS(remote_win_id, remote_offset, comp, new_data,
	                                   res_addr, &local_win->pin, type, req);
}

/* WAIT =================== */

void mpc_lowcomm_rdma_window_RDMA_wait(mpc_lowcomm_request_t *request)
{
	mpc_lowcomm_request_wait(request);
}

/* FENCE =================== */

void mpc_lowcomm_rdma_window_RDMA_fence(mpc_lowcomm_rdma_window_t win_id, mpc_lowcomm_request_t *req)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	if(!win)
	{
		mpc_common_debug_fatal("No such window ID");
	}

	int my_rank = mpc_common_get_task_rank();

	if( (my_rank == win->owner) ||
	    (mpc_lowcomm_rdma_allocmem_is_in_pool(win->start_addr) ) ||
	    (win->access_mode == SCTK_WIN_ACCESS_DIRECT) ||
	    (!mpc_lowcomm_is_remote_rank(win->owner) ) )
	{
		/* Nothing to do all operations are local */
		return;
	}
	else if( (win->is_emulated) ||
	         (win->access_mode == SCTK_WIN_ACCESS_EMULATED) )
	{
		/* In this case we must make sure that the control
		 * message list is flushed before leaving the fence
		 * also the fence is remote we synchornise with a
		 * sendrecv with the remote window */
		if(req)
		{
			sctk_control_message_fence_req(win->owner, win->communicator, req);
		}
		else
		{
			sctk_control_message_fence(win->owner, win->communicator);
		}
	}
	else
	{
		return;
	}
}
