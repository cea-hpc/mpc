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

#ifndef SCTK_WINDOW_H
#define SCTK_WINDOW_H

#include <mpc_common_asm.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_rank.h>

#include "sctk_rail.h"
#include "sctk_window.h"

#include "mpc_thread.h"
#include "mpc_lowcomm_types.h"

/************************************************************************/
/* Definition of an RDMA window                                         */
/************************************************************************/

typedef enum
{
	SCTK_WIN_ACCESS_DIRECT,
	SCTK_WIN_ACCESS_EMULATED,
	SCTK_WIN_ACCESS_AUTO
} mpc_lowcomm_rdma_window_access_mode_t;

struct mpc_lowcomm_rdma_window
{
	int                                   id;
	int                                   remote_id;
	void *                                start_addr;
	size_t                                size;
	size_t                                disp_unit;
	int                                   owner;
	int                                   comm_rank;
	sctk_rail_pin_ctx_t                   pin;
	mpc_lowcomm_communicator_id_t         comm_id;
	mpc_lowcomm_communicator_t            communicator;
	int                                   is_emulated;
	mpc_common_spinlock_t                 lock;
	unsigned int                          refcounter;
	void *                                payload;
	mpc_thread_t                          th;
	int                                   poll;
	mpc_lowcomm_rdma_window_access_mode_t access_mode;
	OPA_int_t                             outgoing_emulated_rma;
	OPA_int_t *                           incoming_emulated_rma;
};

/************************************************************************/
/* Window Counters                                                      */
/************************************************************************/

static inline void mpc_lowcomm_rdma_window_inc_outgoing(struct mpc_lowcomm_rdma_window *win)
{
	OPA_incr_int(&win->outgoing_emulated_rma);
}

static inline void mpc_lowcomm_rdma_window_inc_incoming(struct mpc_lowcomm_rdma_window *win, int rank)
{
	OPA_incr_int(&win->incoming_emulated_rma[rank]);
}

/************************************************************************/
/* Window Translation                                                   */
/************************************************************************/

static inline void mpc_lowcomm_rdma_window_set_access_mode(struct mpc_lowcomm_rdma_window *win,
                                                           mpc_lowcomm_rdma_window_access_mode_t mode)
{
	assume(win);
	win->access_mode = mode;
}

struct sctk_win_translation
{
	struct mpc_lowcomm_rdma_window *win;
	int                             generation;
};

void sctk_win_translation_init(struct sctk_win_translation *wt,
                               struct mpc_lowcomm_rdma_window *win);
void mpc_lowcomm_rdma_window_complete_request(mpc_lowcomm_request_t *req);

extern __thread struct sctk_win_translation __forward_translate;
extern OPA_int_t __rma_generation;

/* Translate to internal type */
struct mpc_lowcomm_rdma_window *__sctk_win_translate(mpc_lowcomm_rdma_window_t win_id);

static inline struct mpc_lowcomm_rdma_window *sctk_win_translate(mpc_lowcomm_rdma_window_t win_id)
{
	int generation = OPA_load_int(&__rma_generation);

	if(generation == __forward_translate.generation)
	{
		if(__forward_translate.win)
		{
			if(__forward_translate.win->id == win_id)
			{
				return __forward_translate.win;
			}
		}
	}

	return __sctk_win_translate(win_id);
}

/************************************************************************/
/* Window Payload                                                       */
/************************************************************************/

static inline void mpc_lowcomm_rdma_window_set_payload(mpc_lowcomm_rdma_window_t win_id,
                                                       void *payload)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	if(!win)
	{
		mpc_common_debug_warning("No such window %d in remote relax", win_id);
		return;
	}

	win->payload = payload;
}

static inline void *mpc_lowcomm_rdma_window_get_payload(mpc_lowcomm_rdma_window_t win_id)
{
	struct mpc_lowcomm_rdma_window *win = sctk_win_translate(win_id);

	if(!win)
	{
		mpc_common_debug_warning("No such window %d in remote relax", win_id);
		return NULL;
	}

	return win->payload;
}

/* Refcounter interactions */
void sctk_win_acquire(mpc_lowcomm_rdma_window_t win_id);
int sctk_win_relax(mpc_lowcomm_rdma_window_t win_id);
void mpc_lowcomm_rdma_window_local_release(mpc_lowcomm_rdma_window_t win_id);

/* Remote window mapping */

struct mpc_lowcomm_rdma_window_map_request
{
	int                       source_rank;
	int                       remote_rank;
	mpc_lowcomm_rdma_window_t win_id;
};

static inline void mpc_lowcomm_rdma_window_map_request_init(struct mpc_lowcomm_rdma_window_map_request *mr, int remote_rank, mpc_lowcomm_rdma_window_t win_id)
{
	mr->source_rank = mpc_common_get_task_rank();
	mr->remote_rank = remote_rank;
	mr->win_id      = win_id;
}

void mpc_lowcomm_rdma_window_init_ht();
void mpc_lowcomm_rdma_window_release_ht();

int mpc_lowcomm_rdma_window_build_from_remote(struct mpc_lowcomm_rdma_window *remote_win_data);

/************************************************************************/
/* Window Emulated Operations  Strutures                                */
/************************************************************************/

struct mpc_lowcomm_rdma_window_emulated_RDMA
{
	int                       source_rank;
	int                       remote_rank;
	size_t                    offset;
	size_t                    size;
	mpc_lowcomm_rdma_window_t win_id;
	char                      payload[0];
};

static inline void mpc_lowcomm_rdma_window_emulated_RDMA_init(struct mpc_lowcomm_rdma_window_emulated_RDMA *erma, int remote_rank, size_t offset, size_t size, mpc_lowcomm_rdma_window_t win_id)
{
	erma->source_rank = mpc_common_get_task_rank();
	erma->remote_rank = remote_rank;
	erma->offset      = offset;
	erma->size        = size;
	erma->win_id      = win_id;
}

struct mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA
{
	struct mpc_lowcomm_rdma_window_emulated_RDMA rdma;
	RDMA_type                                    type;
	RDMA_op                                      op;
	char                                         add[16];
};

static inline void mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA_init(struct mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA *fop, int remote_rank, size_t offset, mpc_lowcomm_rdma_window_t win_id, RDMA_op op, RDMA_type type, void *add)
{
	mpc_lowcomm_rdma_window_emulated_RDMA_init(&fop->rdma, remote_rank, offset, RDMA_type_size(type), win_id);

	fop->type = type;
	fop->op   = op;

	assume(fop->rdma.size <= 16);

	if(add)
	{
		memcpy(fop->add, add, fop->rdma.size);
	}
}

struct mpc_lowcomm_rdma_window_emulated_CAS_RDMA
{
	struct mpc_lowcomm_rdma_window_emulated_RDMA rdma;
	RDMA_type                                    type;
	char                                         comp[16];
	char new[16];
};


static inline void mpc_lowcomm_rdma_window_emulated_CAS_RDMA_init(struct mpc_lowcomm_rdma_window_emulated_CAS_RDMA *fcas, int remote_rank, size_t offset, mpc_lowcomm_rdma_window_t win_id, RDMA_type type, void *cmp, void *new)
{
	mpc_lowcomm_rdma_window_emulated_RDMA_init(&fcas->rdma, remote_rank, offset, RDMA_type_size(type), win_id);

	fcas->type = type;

	assume(fcas->rdma.size <= 16);
	assume(cmp != NULL);
	assume(new != NULL);

	memcpy(fcas->comp, cmp, fcas->rdma.size);
	memcpy(fcas->new, new, fcas->rdma.size);
}

/************************************************************************/
/* Control Messages Handlers                                            */
/************************************************************************/

enum
{
	TAG_RDMA_READ = 7000,
	TAG_RDMA_WRITE,
	TAG_RDMA_WRITE_ACK,
	TAG_RDMA_FETCH_AND_OP,
	TAG_RDMA_CAS,
	TAG_RDMA_MAP,
	TAG_RDMA_ACCUMULATE,
	TAG_RDMA_ACK = 1337,
	TAG_RDMA_FENCE
};

void mpc_lowcomm_rdma_window_map_remote_ctrl_msg_handler(struct mpc_lowcomm_rdma_window_map_request *mr);
void mpc_lowcomm_rdma_window_relax_ctrl_msg_handler(mpc_lowcomm_rdma_window_t win_id);
void mpc_lowcomm_rdma_window_RDMA_emulated_write_ctrl_msg_handler(struct mpc_lowcomm_rdma_window_emulated_RDMA *erma);
void mpc_lowcomm_rdma_window_RDMA_emulated_read_ctrl_msg_handler(struct mpc_lowcomm_rdma_window_emulated_RDMA *erma);
void mpc_lowcomm_rdma_window_RDMA_fetch_and_op_ctrl_msg_handler(struct mpc_lowcomm_rdma_window_emulated_fetch_and_op_RDMA *fop);
void mpc_lowcomm_rdma_window_RDMA_CAS_ctrl_msg_handler(struct mpc_lowcomm_rdma_window_emulated_CAS_RDMA *fcas);

#endif /* SCTK_WINDOW_H */
