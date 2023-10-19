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
static mpc_common_spinlock_t init_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

/* Error handlers */
static OPA_int_t current_errhandler; /* MPC_Errhandler */
static struct MPI_ABI_Errhandler error_handlers[MAX_ERROR_HANDLERS];
static mpc_common_spinlock_t     errorhandlers_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

/* Handles */
static OPA_int_t current_handle;
static struct mpc_common_hashtable handle_context;

/* Error codes */
static OPA_int_t current_error_class;
static OPA_int_t current_error_code;
static struct mpc_common_hashtable error_strings;

void mpc_mpi_err_init()
{
	mpc_common_spinlock_lock(&init_lock);

        if(!init_done)
        {
                /* Error handlers */
                OPA_store_int(&current_errhandler, 4);
                memset(error_handlers, 0, MAX_ERROR_HANDLERS * sizeof(struct MPI_ABI_Errhandler) );

                /* Handles */
                OPA_store_int(&current_handle, SCTK_BOOKED_HANDLES);
                mpc_common_hashtable_init(&handle_context, 64);

                /* Error codes */
                OPA_store_int(&current_error_class, 1024);
                OPA_store_int(&current_error_code, 4);
                mpc_common_hashtable_init(&error_strings, 8);
                init_done = 1;
	}

	mpc_common_spinlock_unlock(&init_lock);
}

static void mpc_mpi_err_init_once()
{
	mpc_common_spinlock_lock(&init_lock);

	if(!init_done)
	{
		mpc_mpi_err_init();
		init_done = 1;
	}

	mpc_common_spinlock_unlock(&init_lock);
}

/************************************************************************/
/* Error Handler                                                        */
/************************************************************************/

_mpc_mpi_errhandler_t _mpc_mpi_errhandler_from_idx(const long errh)
{
	if(errh > 0 && errh <= MAX_ERROR_HANDLERS)
	{
		return error_handlers + errh;
	}

	return (_mpc_mpi_errhandler_t)errh;
}

long _mpc_mpi_errhandler_to_idx(const _mpc_mpi_errhandler_t errh)
{
	if(errh < (_mpc_mpi_errhandler_t)MAX_ERROR_HANDLERS)
	{
		return (long)errh;
	}

	for(unsigned short i = 1; i < MAX_ERROR_HANDLERS; i++)
	{
		if(error_handlers + i == errh)
		{
			return i;
		}
	}

	return (long)errh;
}

bool _mpc_mpi_errhandler_is_valid(const _mpc_mpi_errhandler_t errh)
{
	if(errh == (_mpc_mpi_errhandler_t)1 || errh == (_mpc_mpi_errhandler_t)2 || errh == (_mpc_mpi_errhandler_t)3)
	{
		return true;
	}
	return errh >= error_handlers && errh < error_handlers + MAX_ERROR_HANDLERS;
}

int _mpc_mpi_errhandler_register(_mpc_mpi_generic_errhandler_func_t eh, _mpc_mpi_errhandler_t *errh)
{
	mpc_mpi_err_init_once();

	mpc_common_spinlock_lock(&errorhandlers_lock);
	/* Does it already exist ? */
	for(int i = 0; i < MAX_ERROR_HANDLERS; i++)
	{
		if(error_handlers[i].eh == eh)
		{
			*errh = error_handlers + i;
			error_handlers[i].ref_count++;
			mpc_common_spinlock_unlock(&errorhandlers_lock);
			return MPC_LOWCOMM_SUCCESS;
		}
	}

	/* Find a new slot */
	for(int i = 4; i < MAX_ERROR_HANDLERS; i++)
	{
		if(error_handlers[i].eh == NULL)
		{
			error_handlers[i].eh        = eh;
			error_handlers[i].ref_count = 1;
			*errh = error_handlers + i;
			mpc_common_spinlock_unlock(&errorhandlers_lock);
			return MPC_LOWCOMM_SUCCESS;
		}
	}

	mpc_common_spinlock_unlock(&errorhandlers_lock);
	return 17; /**< MPI_ERR_INTERN */
}

int _mpc_mpi_errhandler_register_on_slot(_mpc_mpi_generic_errhandler_func_t eh,
                                         const int slot)
{
	mpc_mpi_err_init_once();
	/* Save in the HT */
	error_handlers[slot].eh        = eh;
	error_handlers[slot].ref_count = 1;
	/* All ok */
	return 0;
}

_mpc_mpi_generic_errhandler_func_t _mpc_mpi_errhandler_resolve(_mpc_mpi_errhandler_t errh)
{
	mpc_mpi_err_init_once();

	_mpc_mpi_generic_errhandler_func_t ret = NULL;

	/* Direct table lookup */
	if(_mpc_mpi_errhandler_is_valid(errh) )
	{
		errh = _mpc_mpi_errhandler_from_idx( (long)errh);
		ret  = errh->eh;
	}
	/* _mpc_mpi_generic_errhandler_func_t ret = mpc_common_hashtable_get(&error_handlers, errh); */

	mpc_common_nodebug("RET Is %p for %d", ret, errh);

	return ret;
}

/** @brief Checks if an errhandler can be freed (valid and not a predefined)
 *
 *  @param errh    The errhandler to check
 *  @return        true if the errhandler is valid, false otherwise
 */
static inline bool _mpc_mpi_errhandler_can_be_released(const _mpc_mpi_errhandler_t errh)
{
	// In the array but not a predefined errhandler
	return errh >= (error_handlers + 4) && errh < error_handlers + MAX_ERROR_HANDLERS;
}

int _mpc_mpi_errhandler_free_no_lock(_mpc_mpi_errhandler_t errh)
{
	errh = _mpc_mpi_errhandler_from_idx( (long)errh);
	/* Does it is valid ? */
	if(!_mpc_mpi_errhandler_is_valid(errh) )
	{
		/* No then error */
		return 17; /**< May be MPI_ERR_ERRHANDLER instead or MPI_ERR_ARG*/
	}

	if(errh->ref_count > 0)
	{
		errh->ref_count--;
	}

	/* If no more references*/
	if(errh->ref_count == 0)
	{
		if(_mpc_mpi_errhandler_can_be_released(errh) )
		{
                        mpc_common_debug("ERR: can be released");
			memset(errh, 0, sizeof(struct MPI_ABI_Errhandler) );
		}
	}

	/* All done */
	return 0;
}

int _mpc_mpi_errhandler_free(_mpc_mpi_errhandler_t errh)
{
        int ret;
	mpc_mpi_err_init_once();
	mpc_common_spinlock_lock(&errorhandlers_lock);
        ret = _mpc_mpi_errhandler_free_no_lock(errh);
	mpc_common_spinlock_unlock(&errorhandlers_lock);
        return ret;
}


/************************************************************************/
/* MPI Handles                                                          */
/************************************************************************/

static inline uint64_t __handle_hash(sctk_handle id,
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

//static mpc_common_spinlock_t handle_mod_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

static inline struct _mpc_mpi_handle_ctx_s *__ctx_get_no_lock(sctk_handle id, sctk_handle_type type)
{
	return (struct _mpc_mpi_handle_ctx_s *)mpc_common_hashtable_get(&handle_context, __handle_hash(id, type) );
}

static inline struct _mpc_mpi_handle_ctx_s *__ctx_get(sctk_handle id, sctk_handle_type type)
{
	struct _mpc_mpi_handle_ctx_s *ret = NULL;

	ret = __ctx_get_no_lock(id, type);

	if(!ret)
	{
		/* If no such context then create it */
		_mpc_mpi_handle_new_from_id(id, type);
		return __ctx_get_no_lock(id, type);
	}

	return ret;
}

sctk_handle _mpc_mpi_handle_new_from_id(sctk_handle previous_id, sctk_handle_type type)
{
	sctk_handle new_handle_id;

	mpc_mpi_err_init_once();

	new_handle_id = -1;

	if(__ctx_get_no_lock(previous_id, type) == NULL)
	{
		/* Create an unique i */
		new_handle_id = previous_id;

		/* Create the handler payload */
		struct _mpc_mpi_handle_ctx_s *ctx = _mpc_mpi_handle_ctx_new(new_handle_id);

		/* Save in the HT */
		mpc_common_hashtable_set(&handle_context, __handle_hash(new_handle_id, type),
		                         (void *)ctx);
	}

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
	mpc_common_spinlock_lock(&errorhandlers_lock);

	struct _mpc_mpi_handle_ctx_s *hctx = __ctx_get_no_lock(id, type);

	if(!hctx)
	{
		mpc_common_spinlock_unlock(&errorhandlers_lock);
		return -1;
	}

	mpc_common_hashtable_delete(&handle_context, __handle_hash(id, type) );

	mpc_common_spinlock_unlock(&errorhandlers_lock);

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
        mpc_common_spinlock_lock(&errorhandlers_lock);

	struct _mpc_mpi_handle_ctx_s *hctx = __ctx_get(id, type);

	if(!hctx)
	{
                mpc_common_spinlock_unlock(&errorhandlers_lock);
		return SCTK_ERRHANDLER_NULL;
	}

	_mpc_mpi_errhandler_t ret = hctx->handler;

	ret->ref_count++;
	mpc_common_spinlock_unlock(&errorhandlers_lock);

	mpc_common_debug("GET at %p == %d for %d", hctx, hctx->handler, id);

	return ret;
}

int _mpc_mpi_handle_set_errhandler(sctk_handle id, sctk_handle_type type,
                                   _mpc_mpi_errhandler_t errh)
{
	mpc_common_spinlock_lock(&errorhandlers_lock);
	struct _mpc_mpi_handle_ctx_s *hctx = __ctx_get(id, type);

	if(!hctx)
	{
                mpc_common_spinlock_unlock(&errorhandlers_lock);
		return 1;
	}

	errh = _mpc_mpi_errhandler_from_idx( (long)errh);

	_mpc_mpi_errhandler_free_no_lock(hctx->handler);
	hctx->handler = errh;
	errh->ref_count++;
	mpc_common_spinlock_unlock(&errorhandlers_lock);

	mpc_common_debug("SET %d at %p <= %p for %d", errh, hctx, hctx->handler, id);

	return 0;
}
