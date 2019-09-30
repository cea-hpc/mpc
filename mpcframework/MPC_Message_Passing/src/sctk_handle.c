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
#include "sctk_handle.h"
#include "mpc_common_asm.h"
#include "sctk_debug.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"
#include "sctk_thread.h"
#include <sctk_inter_thread_comm.h>

/************************************************************************/
/* Storage for this whole file                                          */
/************************************************************************/

static int init_done = 0;
static mpc_common_spinlock_t init_lock = 0;

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

static void _mpc_mpi_err_init() {
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

static void mpc_mpi_err_init_once() {
  mpc_common_spinlock_lock(&init_lock);

  if (!init_done) {
    _mpc_mpi_err_init();
    init_done = 1;
  }

  mpc_common_spinlock_unlock(&init_lock);
}

/************************************************************************/
/* Error Handler                                                        */
/************************************************************************/

int sctk_errhandler_register(sctk_generic_handler eh, sctk_errhandler_t *errh) {
  mpc_mpi_err_init_once();
  /* Create an unique id */
  sctk_errhandler_t new_id =
      OPA_fetch_and_add_int(&current_errhandler, 1);
  /* Give it to the iface */
  *errh = (sctk_errhandler_t)new_id;
  /* Save in the HT */
  sctk_nodebug("REGISTER Is %p for %d", eh, *errh);
  mpc_common_hashtable_set(&error_handlers, new_id, (void *)eh);
  /* All ok */
  return 0;
}

int sctk_errhandler_register_on_slot(sctk_generic_handler eh,
                                     sctk_errhandler_t slot) {
  mpc_mpi_err_init_once();
  /* Save in the HT */
  sctk_nodebug("REGISTER Is %p for %d", eh, *errh);
  mpc_common_hashtable_set(&error_handlers, slot, (void *)eh);
  /* All ok */
  return 0;
}

sctk_generic_handler sctk_errhandler_resolve(sctk_errhandler_t errh) {
  mpc_mpi_err_init_once();
  /* Direct HT lookup */

  sctk_generic_handler ret = mpc_common_hashtable_get(&error_handlers, errh);

  sctk_nodebug("RET Is %p for %d", ret, errh);

  return ret;
}

int sctk_errhandler_free(sctk_errhandler_t errh) {
  mpc_mpi_err_init_once();
  /* Does it exists  ? */
  sctk_generic_handler eh = sctk_errhandler_resolve(errh);

  if (!eh) {
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
                                                sctk_handle_type type) {
  uint64_t rank = mpc_common_get_task_rank();

  uint64_t ret = type;
  ret |= id << 16;
  ret |= rank << 32;

  sctk_nodebug("RET (%d %d %d) %ld", mpc_common_get_task_rank(), id, type, ret);
  return ret;
}

struct sctk_handle_context *sctk_handle_context_new(sctk_handle id) {
  struct sctk_handle_context *ret =
      sctk_malloc(sizeof(struct sctk_handle_context));
  assume(ret != NULL);

  ret->id = id;
  ret->handler = SCTK_ERRHANDLER_NULL;

  return ret;
}

static mpc_common_spinlock_t handle_mod_lock = 0;

struct sctk_handle_context *sctk_handle_context_no_lock(sctk_handle id,
                                                        sctk_handle_type type) {
  return (struct sctk_handle_context *)mpc_common_hashtable_get(&handle_context,
                                                 sctk_handle_compute(id, type));
}

struct sctk_handle_context *sctk_handle_context(sctk_handle id,
                                                sctk_handle_type type) {
  struct sctk_handle_context *ret = NULL;

  mpc_common_spinlock_lock_yield(&handle_mod_lock);
  ret = sctk_handle_context_no_lock(id, type);
  mpc_common_spinlock_unlock(&handle_mod_lock);

  if (!ret) {
    /* If no such context then create it */
    sctk_handle_new_from_id(id, type);
    return sctk_handle_context(id, type);
  }

  return ret;
}

sctk_handle sctk_handle_new_from_id(int previous_id, sctk_handle_type type) {
  sctk_handle new_handle_id;

  mpc_mpi_err_init_once();

  mpc_common_spinlock_lock_yield(&handle_mod_lock);

  if (sctk_handle_context_no_lock(previous_id, type) == NULL) {
    /* Create an unique id */
    new_handle_id = previous_id;

    /* Create the handler payload */
    struct sctk_handle_context *ctx = sctk_handle_context_new(new_handle_id);

    /* Save in the HT */
    mpc_common_hashtable_set(&handle_context, sctk_handle_compute(new_handle_id, type),
              (void *)ctx);
  }

  mpc_common_spinlock_unlock(&handle_mod_lock);

  /* All ok return the handle */
  return new_handle_id;
}

sctk_handle sctk_handle_new(sctk_handle_type type) {
  sctk_handle new_handle_id;

  mpc_mpi_err_init_once();

  /* Create an unique id */
  new_handle_id = OPA_fetch_and_add_int(&current_handle, 1);

  return sctk_handle_new_from_id(new_handle_id, type);
}

int sctk_handle_free(sctk_handle id, sctk_handle_type type) {

  mpc_common_spinlock_lock_yield(&handle_mod_lock);

  struct sctk_handle_context *hctx = sctk_handle_context_no_lock(id, type);

  if (!hctx) {
    mpc_common_spinlock_unlock(&handle_mod_lock);
    return MPC_ERR_ARG;
  }

  mpc_common_hashtable_delete(&handle_context, sctk_handle_compute(id, type));

  mpc_common_spinlock_unlock(&handle_mod_lock);

  sctk_handle_context_release(hctx);

  return 0;
}

int sctk_handle_context_release(struct sctk_handle_context *hctx) {
  memset(hctx, 0, sizeof(sctk_handle_context));
  sctk_free(hctx);

  return 0;
}

sctk_errhandler_t sctk_handle_get_errhandler(sctk_handle id,
                                             sctk_handle_type type) {
  struct sctk_handle_context *hctx = sctk_handle_context(id, type);

  if (!hctx) {
    return SCTK_ERRHANDLER_NULL;
  }

  sctk_nodebug("GET at %p == %d for %d", hctx, hctx->handler, id);

  return hctx->handler;
}

int sctk_handle_set_errhandler(sctk_handle id, sctk_handle_type type,
                               sctk_errhandler_t errh) {
  struct sctk_handle_context *hctx = sctk_handle_context(id, type);

  if (!hctx) {
    return 1;
  }

  hctx->handler = errh;

  sctk_nodebug("SET %d at %p <= %p for %d", errh, hctx, hctx->handler, id);

  return 0;
}
