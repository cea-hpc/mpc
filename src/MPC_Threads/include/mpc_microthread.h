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
#ifndef __mpcmicrothread__H
#define __mpcmicrothread__H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <errno.h>
#include <mpc_thread.h>

/* Definition of a function for microthread */
#define MPC_MICROTHREAD_FUNC_T    (volatile void *(*)(void *) )

//extern mpc_thread_keys_t sctk_microthread_key;

typedef struct sctk_microthread_vp_s   sctk_microthread_vp_t;

/* Structure handling the virtual processors */
typedef struct sctk_microthread_s
{
	/* Set of VPs for this instance of microthread */
	sctk_microthread_vp_t *__list;
	/* Number of VPs (size of '__list') */
	long                   __nb_vp;
	/* Is this microthread instance enable? */
	/* Note: set this variable to 0 kills the VPs */
	volatile int           enable;
} sctk_microthread_t;


/*
 * NOLINTBEGIN(clang-diagnostic-unused-function):
 * In a public header, exposed to applications.
 */
/* Function initializing a 'microthread' structure */
static inline void sctk_microthread_init_microthread_t(sctk_microthread_t *self)
{
	/* No VPs so far */
	self->__list  = NULL;
	self->__nb_vp = 0;
	/* Ready to accept VPs and being scheduled */
	self->enable = 1;
}
/* NOLINTEND(clang-diagnostic-unused-function) */

/* Application Programming Interface for Microthreads */

/* Initialization of a microthread instance */
int sctk_microthread_init(long nb_vp, sctk_microthread_t *self);

#define MPC_MICROTHREAD_LAST_TASK       (-2)
#define MPC_MICROTHREAD_NO_TASK_INFO    (-1)

/* Enqueue an operation (func,arg) to a specific VP */

/* val is the expected number of tasks for this specific VP.
 * if equal to MPC_MICROTHREAD_LAST_TASK, this task will be considered as the
 * last one
 */
int sctk_microthread_add_task(void *(*func)(void *), void *arg,
                              long vp, long *step,
                              sctk_microthread_t *task, int val);

#define MPC_MICROTHREAD_DONT_WAKE_VPS    (1)
#define MPC_MICROTHREAD_WAKE_VPS         (-1)

/* Launch the execution of this microthread instance */
/* 'val' -> (MPC_MICROTHREAD_DONT_WAKE_VPS|MPC_MICROTHREAD_WAKE_VPS) */
int sctk_microthread_parallel_exec(sctk_microthread_t *task, int val);

/* TODO: just put the enable flag to 0, then this VP dies?
 * int sctk_microthread_finalize_vp (long vp, sctk_microthread_t * self);
 */


#ifdef __cplusplus
}
#endif

#endif
