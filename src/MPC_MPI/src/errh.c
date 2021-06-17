#include "error.h"

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
#include "errh.h"

#include <mpc_common_asm.h>
#include <mpc_common_rank.h>
#include "mpc_common_debug.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"
#include "mpc_thread.h"
#include <comm.h>

#include <sctk_alloc.h>

/************************************************************************/
/* Storage for this whole file                                          */
/************************************************************************/

static int init_done = 0;
static mpc_common_spinlock_t init_lock = SCTK_SPINLOCK_INITIALIZER;

/* Error handlers */
static OPA_int_t current_errhandler;
static struct mpc_common_hashtable error_handlers;

/* Handles */
static OPA_int_t current_handle;
static struct mpc_common_hashtable handle_context;

/* Error codes */
static OPA_int_t current_error_class;
static OPA_int_t current_error_code;
static struct mpc_common_hashtable error_strings;

static void _mpc_mpi_err_init()
{
	/* Error handlers */
	OPA_store_int(&current_errhandler, 100);
	mpc_common_hashtable_init(&error_handlers, 8);

	/* Handles */
	OPA_store_int(&current_handle, SCTK_BOOKED_HANDLES);
	mpc_common_hashtable_init(&handle_context, 64);

	/* Error codes */
	OPA_store_int(&current_error_class, 1024);
	OPA_store_int(&current_error_code, 100);
	mpc_common_hashtable_init(&error_strings, 8);
}

static void mpc_mpi_err_init_once()
{
	mpc_common_spinlock_lock(&init_lock);

	if(!init_done)
	{
		_mpc_mpi_err_init();
		init_done = 1;
	}

	mpc_common_spinlock_unlock(&init_lock);
}

/************************************************************************/
/* Error Handler                                                        */
/************************************************************************/

int _mpc_mpi_errhandler_register(_mpc_mpi_generic_errhandler_func_t eh, _mpc_mpi_errhandler_t *errh)
{
	mpc_mpi_err_init_once();
	/* Create an unique id */
	_mpc_mpi_errhandler_t new_id =
		OPA_fetch_and_add_int(&current_errhandler, 1);
	/* Give it to the iface */
	*errh = (_mpc_mpi_errhandler_t)new_id;
	/* Save in the HT */
	mpc_common_nodebug("REGISTER Is %p for %d", eh, *errh);
	mpc_common_hashtable_set(&error_handlers, new_id, (void *)eh);
	/* All ok */
	return 0;
}

int _mpc_mpi_errhandler_register_on_slot(_mpc_mpi_generic_errhandler_func_t eh,
                                     _mpc_mpi_errhandler_t slot)
{
	mpc_mpi_err_init_once();
	/* Save in the HT */
	mpc_common_hashtable_set(&error_handlers, slot, (void *)eh);
	/* All ok */
	return 0;
}

_mpc_mpi_generic_errhandler_func_t _mpc_mpi_errhandler_resolve(_mpc_mpi_errhandler_t errh)
{
	mpc_mpi_err_init_once();
	/* Direct HT lookup */

	_mpc_mpi_generic_errhandler_func_t ret = mpc_common_hashtable_get(&error_handlers, errh);

	mpc_common_nodebug("RET Is %p for %d", ret, errh);

	return ret;
}

int _mpc_mpi_errhandler_free(_mpc_mpi_errhandler_t errh)
{
	mpc_mpi_err_init_once();
	/* Does it exists  ? */
	_mpc_mpi_generic_errhandler_func_t eh = _mpc_mpi_errhandler_resolve(errh);

	if(!eh)
	{
		/* No then error */
		return 1;
	}

	/* If present delete */
	mpc_common_hashtable_delete(&error_handlers, errh);

	/* All done */
	return 0;
}

/************************************************************************/
/* MPI Handles                                                          */
/************************************************************************/

static inline uint64_t sctk_handle_compute(sctk_handle id,
                                           sctk_handle_type type)
{
	uint64_t rank = mpc_common_get_task_rank();

	uint64_t ret = type;

	ret |= id << 16;
	ret |= rank << 32;

	mpc_common_nodebug("RET (%d %d %d) %ld", mpc_common_get_task_rank(), id, type, ret);
	return ret;
}

struct _mpc_mpi_handle_ctx_s *_mpc_mpi_handle_ctx_new(sctk_handle id)
{
	struct _mpc_mpi_handle_ctx_s *ret =
		sctk_malloc(sizeof(struct _mpc_mpi_handle_ctx_s) );

	assume(ret != NULL);

	ret->id      = id;
	ret->handler = SCTK_ERRHANDLER_NULL;

	return ret;
}

static mpc_common_spinlock_t handle_mod_lock = SCTK_SPINLOCK_INITIALIZER;

static inline  struct _mpc_mpi_handle_ctx_s *__ctx_get_no_lock(sctk_handle id, sctk_handle_type type)
{
	return (struct _mpc_mpi_handle_ctx_s *)mpc_common_hashtable_get(&handle_context, sctk_handle_compute(id, type) );
}

static inline struct _mpc_mpi_handle_ctx_s *__ctx_get(sctk_handle id, sctk_handle_type type)
{
	struct _mpc_mpi_handle_ctx_s *ret = NULL;

	mpc_common_spinlock_lock_yield(&handle_mod_lock);
	ret = __ctx_get_no_lock(id, type);
	mpc_common_spinlock_unlock(&handle_mod_lock);

	if(!ret)
	{
		/* If no such context then create it */
		_mpc_mpi_handle_new_from_id(id, type);
		return __ctx_get(id, type);
	}

	return ret;
}

sctk_handle _mpc_mpi_handle_new_from_id(sctk_handle previous_id, sctk_handle_type type)
{
	sctk_handle new_handle_id;

	mpc_mpi_err_init_once();

	mpc_common_spinlock_lock_yield(&handle_mod_lock);

	new_handle_id = -1;

	if(__ctx_get_no_lock(previous_id, type) == NULL)
	{
		/* Create an unique id */
		new_handle_id = previous_id;

		/* Create the handler payload */
		struct _mpc_mpi_handle_ctx_s *ctx = _mpc_mpi_handle_ctx_new(new_handle_id);

		/* Save in the HT */
		mpc_common_hashtable_set(&handle_context, sctk_handle_compute(new_handle_id, type),
		                         (void *)ctx);
	}

	mpc_common_spinlock_unlock(&handle_mod_lock);

	/* All ok return the handle */
	return new_handle_id;
}

sctk_handle _mpc_mpi_handle_new(sctk_handle_type type)
{
	sctk_handle new_handle_id;

	mpc_mpi_err_init_once();

	/* Create an unique id */
	new_handle_id = OPA_fetch_and_add_int(&current_handle, 1);

	return _mpc_mpi_handle_new_from_id(new_handle_id, type);
}

int _mpc_mpi_handle_free(sctk_handle id, sctk_handle_type type)
{
	mpc_common_spinlock_lock_yield(&handle_mod_lock);

	struct _mpc_mpi_handle_ctx_s *hctx = __ctx_get_no_lock(id, type);

	if(!hctx)
	{
		mpc_common_spinlock_unlock(&handle_mod_lock);
		return -1;
	}

	mpc_common_hashtable_delete(&handle_context, sctk_handle_compute(id, type) );

	mpc_common_spinlock_unlock(&handle_mod_lock);

	_mpc_mpi_handle_ctx_release(hctx);

	return 0;
}

int _mpc_mpi_handle_ctx_release(struct _mpc_mpi_handle_ctx_s *hctx)
{
	memset(hctx, 0, sizeof(struct _mpc_mpi_handle_ctx_s) );
	sctk_free(hctx);

	return 0;
}

_mpc_mpi_errhandler_t _mpc_mpi_handle_get_errhandler(sctk_handle id,
                                             sctk_handle_type type)
{
	struct _mpc_mpi_handle_ctx_s *hctx = __ctx_get(id, type);

	if(!hctx)
	{
		return SCTK_ERRHANDLER_NULL;
	}

	mpc_common_nodebug("GET at %p == %d for %d", hctx, hctx->handler, id);

	return hctx->handler;
}

int _mpc_mpi_handle_set_errhandler(sctk_handle id, sctk_handle_type type,
                               _mpc_mpi_errhandler_t errh)
{
	struct _mpc_mpi_handle_ctx_s *hctx = __ctx_get(id, type);

	if(!hctx)
	{
		return 1;
	}

	hctx->handler = errh;

	mpc_common_nodebug("SET %d at %p <= %p for %d", errh, hctx, hctx->handler, id);

	return 0;
}
