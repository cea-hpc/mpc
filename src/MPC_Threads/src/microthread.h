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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __mpcmicrothread_internal__H
#define __mpcmicrothread_internal__H

#ifdef __cplusplus
extern "C"
{
#endif

#include <mpc_microthread.h>

#include "sctk_context.h"

/* Max number of microthreads to be scheduled per microVP */
#define MAX_OP_LIST    512

/* An operation (or a microthread) */
typedef struct
{
	/* Function to call when scheduling this micro thread */
	volatile void *(*func)(void *);
	/* Generic argument of the previous function */
	volatile void *arg;
} sctk_microthread_op_t;

/* Structure handling one virtual processor */
struct sctk_microthread_vp_s
{
	/* Context including registers, stack pointer, ... */
	sctk_mctx_t           vp_context;
	/* What remains to be executed on this VP (busy waiting) */
	volatile long         to_do_list;
	/* Is this VP still alive? (Not really used) */
	volatile long         enable;
	/* Is this microVP running or not? */
	volatile int          to_run;
	/* Padding to avoid misalignment of to_do_list/to_do_list_next */
	char                  foo[64];
	/* What remains to be executed on this VP (no busy waiting) */
	volatile long         to_do_list_next;
	/* Queue of microthreads on this VP */
	sctk_microthread_op_t op_list[MAX_OP_LIST];
	/* Thread ID */
	mpc_thread_t          pid;
	/* Padding to force a right alignment (32B) on ia64 */
	char                  foo2[8];

	/* TODO remove this part (should be replaced by 'levels') */
	long                  barrier;
	long                  barrier_done;
};

#ifdef __cplusplus
}
#endif

#endif
