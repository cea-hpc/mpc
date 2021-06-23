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
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_PTHREAD_COMPATIBLE_H_
#define __SCTK_PTHREAD_COMPATIBLE_H_

#include <mpc_common_spinlock.h>

/*    _
 *   / \
 *  / | \        **************************************
 * /  |  \       |  Structures MUST multiple of int.  |
 * /       \      **************************************
 * /    *    \
 * /___________\
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif

/*Threads definitions */

struct _mpc_thread_ethread_per_thread_s;
typedef struct _mpc_thread_ethread_per_thread_s * _mpc_thread_ethread_t;

struct _mpc_thread_ethread_attr_intern_s;
typedef struct
{
	struct _mpc_thread_ethread_attr_intern_s *ptr;
} _mpc_thread_ethread_attr_t;

/*Condition definition */

struct _mpc_thread_ethread_mutex_cell_s;
typedef struct
{
	mpc_common_spinlock_t                             lock;
	volatile int                                      is_init;
	volatile struct _mpc_thread_ethread_mutex_cell_s *list;
	volatile struct _mpc_thread_ethread_mutex_cell_s *list_tail;
} _mpc_thread_ethread_cond_t;
#define SCTK_ETHREAD_COND_INIT    { MPC_COMMON_SPINLOCK_INITIALIZER, 0, NULL, NULL }

/*condition attributes */

typedef struct
{
	short pshared;
	short clock;
} _mpc_thread_ethread_condattr_t;

/*Semaphore management */

typedef struct
{
	volatile int                                      lock;
	mpc_common_spinlock_t                             spinlock;
	volatile struct _mpc_thread_ethread_mutex_cell_s *list;
	volatile struct _mpc_thread_ethread_mutex_cell_s *list_tail;
} _mpc_thread_ethread_sem_t;
#define STCK_ETHREAD_SEM_INIT    { 0, MPC_COMMON_SPINLOCK_INITIALIZER, NULL, NULL }

/*Mutex definition */

struct _mpc_thread_ethread_mutex_cell_s;

typedef struct
{
	volatile struct _mpc_thread_ethread_per_thread_s *owner;
	volatile struct _mpc_thread_ethread_mutex_cell_s *list;
	volatile struct _mpc_thread_ethread_mutex_cell_s *list_tail;
	mpc_common_spinlock_t                             spinlock;
	volatile unsigned int                             lock;
	unsigned int                                      type;
} _mpc_thread_ethread_mutex_t;
#define SCTK_ETHREAD_MUTEX_INIT              { NULL, NULL, NULL, MPC_COMMON_SPINLOCK_INITIALIZER, 0, SCTK_THREAD_MUTEX_DEFAULT }
#define SCTK_ETHREAD_MUTEX_RECURSIVE_INIT    { NULL, NULL, NULL, MPC_COMMON_SPINLOCK_INITIALIZER, 0, SCTK_THREAD_MUTEX_RECURSIVE }

typedef struct
{
	unsigned short kind;
} _mpc_thread_ethread_mutexattr_t;

/*Rwlock*/

typedef struct _mpc_thread_ethread_rwlock_cell_s
{
	struct _mpc_thread_ethread_rwlock_cell_s *next;
	struct _mpc_thread_ethread_per_thread_s * my_self;
	volatile int                              wake;
	volatile unsigned short                   type;
} _mpc_thread_ethread_rwlock_cell_t;


typedef struct
{
	mpc_common_spinlock_t                       spinlock;
	volatile _mpc_thread_ethread_rwlock_cell_t *list;
	volatile _mpc_thread_ethread_rwlock_cell_t *list_tail;
	volatile unsigned int                       lock;
	volatile unsigned short                     current;
	volatile unsigned short                     wait;
} _mpc_thread_ethread_rwlock_t;

#define SCTK_ETHREAD_RWLOCK_INIT    { MPC_COMMON_SPINLOCK_INITIALIZER, NULL, NULL, 0, SCTK_RWLOCK_ALONE, SCTK_RWLOCK_NO_WR_WAIT }
#define SCTK_RWLOCK_READ            1
#define SCTK_RWLOCK_WRITE           2
#define SCTK_RWLOCK_ALONE           0
#define SCTK_RWLOCK_NO_WR_WAIT      0
#define SCTK_RWLOCK_WR_WAIT         1

/*RwLock attributes*/
typedef struct
{
	int pshared;
} _mpc_thread_ethread_rwlockattr_t;

/*barrier*/

typedef struct
{
	mpc_common_spinlock_t                             spinlock;
	volatile struct _mpc_thread_ethread_mutex_cell_s *list;
	volatile struct _mpc_thread_ethread_mutex_cell_s *list_tail;
	volatile unsigned int                             nb_max;
	volatile unsigned int                             lock;
} _mpc_thread_ethread_barrier_t;

/*barrier attributes*/

typedef _mpc_thread_ethread_rwlockattr_t   _mpc_thread_ethread_barrierattr_t;

#ifdef __cplusplus
}
#endif

#endif
