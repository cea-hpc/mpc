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
#ifndef __SCTK_THREAD_GENERIC_H_
#define __SCTK_THREAD_GENERIC_H_



#include <mpc_thread_ng_engine.h>

#include "mpc_common_debug.h"
#include "mpc_thread.h"


#include <ng_engine_scheduler.h>
#include <uthash.h>


/***********************
* THREAD KIND SUPPORT *
***********************/

/**
 * @brief  the member mask is a mask of bits
 *
 */
typedef struct _mpc_threads_ng_engine_kind_s
{
	unsigned int mask;
	int          priority;
} _mpc_threads_ng_engine_kind_t;

#define _mpc_threads_ng_engine_kind_init \
	{ 0, -1 }


/* THREAD KIND SETTERS */

/**
 * @brief set kind to the current thread
 *
 * @param kind type of the thread
 */
void _mpc_threads_ng_engine_kind_set_self(_mpc_threads_ng_engine_kind_t kind);


/* THREAD KIND GETTERS */

/**
 * @brief get kind
 *
 * @return a copy of the current thread's kind
 */
_mpc_threads_ng_engine_kind_t _mpc_threads_ng_engine_kind_get();


/****************
* KEYS SUPPORT *
****************/

typedef struct
{
	const void *keys[SCTK_THREAD_KEYS_MAX + 1];
}_mpc_threads_ng_engine_keys_t;

/* Used in scheduler */
void _mpc_threads_ng_engine_key_init_thread(_mpc_threads_ng_engine_keys_t *keys);

/***************************************/
/* MUTEX                               */
/***************************************/

/* Values of kind attribute */

/*#define SCTK_THREAD_MUTEX_NORMAL PTHREAD_MUTEX_NORMAL
 * #define SCTK_THREAD_MUTEX_ERRORCHECK PTHREAD_MUTEX_ERRORCHECK
 * #define SCTK_THREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE
 #define SCTK_THREAD_MUTEX_DEFAULT PTHREAD_MUTEX_DEFAULT*/

/*schedpolicy*/

/*#define SCTK_ETHREAD_SCHED_OTHER 0
 * #define SCTK_ETHREAD_SCHED_RR 1
 #define SCTK_ETHREAD_SCHED_FIFO 2*/

typedef struct _mpc_threads_ng_engine_mutex_cell_s
{
	_mpc_threads_ng_engine_scheduler_t *        sched;
	struct _mpc_threads_ng_engine_mutex_cell_s *prev, *next;
}_mpc_threads_ng_engine_mutex_cell_t;

typedef struct _mpc_threads_ng_engine_mutex_s
{
	volatile _mpc_threads_ng_engine_scheduler_t *owner;
	mpc_common_spinlock_t                     lock;
	int                                       type;
	int                                       nb_call;
	_mpc_threads_ng_engine_mutex_cell_t *        blocked;
}_mpc_threads_ng_engine_mutex_t;

#define SCTK_THREAD_GENERIC_MUTEX_INIT    { NULL, MPC_COMMON_SPINLOCK_INITIALIZER, 0, 0, NULL }


typedef struct _mpc_threads_ng_engine_mutexattr_s
{
	volatile int attrs;
}_mpc_threads_ng_engine_mutexattr_t;

#define SCTK_THREAD_GENERIC_MUTEXATTR_INIT    { 0 }

/***************************************/
/* CONDITIONS                          */
/***************************************/

typedef struct _mpc_threads_ng_engine_cond_cell_s
{
	_mpc_threads_ng_engine_scheduler_t *       sched;
	_mpc_threads_ng_engine_mutex_t *           binded;
	struct _mpc_threads_ng_engine_cond_cell_s *prev, *next;
}_mpc_threads_ng_engine_cond_cell_t;

typedef struct
{
	mpc_common_spinlock_t            lock;
	_mpc_threads_ng_engine_cond_cell_t *blocked;
	clockid_t                        clock_id;
}_mpc_threads_ng_engine_cond_t;

#define SCTK_THREAD_GENERIC_COND_INIT    { MPC_COMMON_SPINLOCK_INITIALIZER, NULL, 0 }

typedef struct _mpc_threads_ng_engine_condattr_s
{
	int attrs;
}_mpc_threads_ng_engine_condattr_t;

#define SCTK_THREAD_GENERIC_CONDATTR_INIT    { 0 }

/***************************************/
/* SEMAPHORES                          */
/***************************************/


typedef struct _mpc_threads_ng_engine_sem_s
{
	volatile unsigned int             lock;
	mpc_common_spinlock_t             spinlock;
	_mpc_threads_ng_engine_mutex_cell_t *list;
}_mpc_threads_ng_engine_sem_t;

#define SCTK_THREAD_GENERIC_SEM_INIT    { 0, MPC_COMMON_SPINLOCK_INITIALIZER, NULL }

typedef struct _mpc_threads_ng_engine_sem_named_list_s
{
	char *                                       name;
	volatile int                                 nb;
	volatile int                                 unlink;
	volatile mode_t                              mode;
	_mpc_threads_ng_engine_sem_t *                  sem;
	struct _mpc_threads_ng_engine_sem_named_list_s *prev, *next;
}_mpc_threads_ng_engine_sem_named_list_t;

/***************************************/
/* READ/WRITE LOCKS                    */
/***************************************/

#define SCTK_RWLOCK_READ             1
#define SCTK_RWLOCK_WRITE            2
#define SCTK_RWLOCK_TRYREAD          3
#define SCTK_RWLOCK_TRYWRITE         4
#define SCTK_RWLOCK_ALONE            0
#define SCTK_RWLOCK_NO_WR_WAITING    0
#define SCTK_RWLOCK_WR_WAITING       1

typedef enum
{
	SCTK_UNINITIALIZED, SCTK_INITIALIZED, SCTK_DESTROYED
}sctk_rwlock_status_t;

typedef struct _mpc_threads_ng_engine_rwlock_cell_s
{
	_mpc_threads_ng_engine_scheduler_t *         sched;
	volatile unsigned int                     type;
	struct _mpc_threads_ng_engine_rwlock_cell_s *prev, *next;
}_mpc_threads_ng_engine_rwlock_cell_t;

typedef struct _mpc_threads_ng_engine_rwlockattr_s
{
	volatile int pshared;
}_mpc_threads_ng_engine_rwlockattr_t;

#define SCTK_THREAD_GENERIC_RWLOCKATTR_INIT    { SCTK_THREAD_PROCESS_PRIVATE }

typedef struct _mpc_threads_ng_engine_rwlock_s
{
	mpc_common_spinlock_t                       lock;
	volatile sctk_rwlock_status_t               status;
	volatile unsigned int                       count;
	volatile unsigned int                       reads_count;
	volatile unsigned int                       current;
	volatile unsigned int                       wait;
	volatile _mpc_threads_ng_engine_scheduler_t *  writer;
	volatile _mpc_threads_ng_engine_rwlock_cell_t *readers;
	_mpc_threads_ng_engine_rwlock_cell_t *         waiting;
}_mpc_threads_ng_engine_rwlock_t;

#define SCTK_THREAD_GENERIC_RWLOCK_INIT    { MPC_COMMON_SPINLOCK_INITIALIZER, SCTK_UNINITIALIZED, 0, 0, SCTK_RWLOCK_ALONE, SCTK_RWLOCK_NO_WR_WAITING, NULL, NULL, NULL }

typedef struct
{
	_mpc_threads_ng_engine_rwlock_t *rwlock;
	UT_hash_handle                hh;
}mpc_thread_rwlock_in_use_t;

/***************************************/
/* THREAD BARRIER                      */
/***************************************/

typedef struct _mpc_threads_ng_engine_barrier_cell_s
{
	_mpc_threads_ng_engine_scheduler_t *          sched;
	struct _mpc_threads_ng_engine_barrier_cell_s *prev, *next;
}_mpc_threads_ng_engine_barrier_cell_t;

typedef struct _mpc_threads_ng_engine_barrier_s
{
	mpc_common_spinlock_t               lock;
	volatile int                        nb_max;
	volatile int                        nb_current;
	_mpc_threads_ng_engine_barrier_cell_t *blocked;
}_mpc_threads_ng_engine_barrier_t;

#define SCTK_THREAD_GENERIC_BARRIER_INIT    { MPC_COMMON_SPINLOCK_INITIALIZER, 0, 0, NULL }

typedef struct _mpc_threads_ng_engine_barrierattr_s
{
	volatile int pshared;
}_mpc_threads_ng_engine_barrierattr_t;

#define SCTK_THREAD_GENERIC_BARRIERATTR_INIT    { SCTK_THREAD_PROCESS_PRIVATE }

/***************************************/
/* THREAD SPINLOCK                     */
/***************************************/

typedef enum
{
	sctk_spin_unitialized,
	sctk_spin_initialized,
	sctk_spin_destroyed
} sctk_spin_state;

typedef struct _mpc_threads_ng_engine_spinlock_s
{
	mpc_common_spinlock_t            lock;
	sctk_spin_state                  state;
	_mpc_threads_ng_engine_scheduler_t *owner;
}_mpc_threads_ng_engine_spinlock_t;

#define SCTK_THREAD_GENERIC_SPINLOCK_INIT    { MPC_COMMON_SPINLOCK_INITIALIZER, sctk_spin_unitialized, NULL }

#define SCTK_SPIN_DELAY                      10



typedef struct _mpc_threads_ng_engine_intern_attr_s
{
	int                          scope;
	int                          detachstate;
	int                          schedpolicy; // can I use this field ?
	int                          inheritsched;
	int                          nb_wait_for_join;
	volatile int                 cancel_state;
	volatile int                 cancel_type;
	volatile int                 cancel_status;
	char *                       user_stack;
	char *                       stack;
	size_t                       stack_size;
	size_t                       stack_guardsize;
	void *(*start_routine)(void *);
	void *                       arg;
	void *                       return_value;
	int                          bind_to;
	int                          polling;                                         /* ----------------------------------------- */
	void *                       _mpc_threads_ng_engine_pthread_blocking_lock_table; /* |BARRIER|COND|MUTEX|RWLOCK|SEM|TASK LOCK| */
	mpc_thread_rwlock_in_use_t *rwlocks_owned;                                   /* ----------------------------------------- */
	mpc_common_spinlock_t        spinlock;
	volatile int                 nb_sig_pending;
	volatile int                 nb_sig_treated;

	_mpc_threads_ng_engine_kind_t   kind;
	int                          basic_priority;
	int                          current_priority;

	// timers to have the time of the threads
	double                       timestamp_threshold;
	double                       timestamp_base;
	double                       timestamp_count;
	double                       timestamp_begin;
	double                       timestamp_end;
	// end of init macro

	volatile sigset_t            old_thread_sigset;
	volatile sigset_t            thread_sigset;
	volatile sigset_t            sa_sigset_mask;
	volatile int                 thread_sigpending[SCTK_NSIG];
}_mpc_threads_ng_engine_intern_attr_t;

typedef struct
{
	_mpc_threads_ng_engine_intern_attr_t *ptr;
} _mpc_threads_ng_engine_attr_t;

typedef struct _mpc_threads_ng_engine_p_s
{
	_mpc_threads_ng_engine_scheduler_t   sched;
	_mpc_threads_ng_engine_keys_t        keys;
	_mpc_threads_ng_engine_intern_attr_t attr;
} _mpc_threads_ng_engine_p_t;

typedef _mpc_threads_ng_engine_p_t * _mpc_threads_ng_engine_t;
void _mpc_threads_ng_engine_self_set(_mpc_threads_ng_engine_t th);
_mpc_threads_ng_engine_t _mpc_threads_ng_engine_self();
int
_mpc_threads_ng_engine_user_create(_mpc_threads_ng_engine_t *threadp,
                                _mpc_threads_ng_engine_attr_t *attr,
                                void *(*start_routine)(void *), void *arg);
int
_mpc_threads_ng_engine_attr_init(_mpc_threads_ng_engine_attr_t *attr);

void _mpc_threads_ng_engine_check_signals(int select);

void _mpc_threads_ng_engine_handle_zombies(
        _mpc_threads_ng_engine_scheduler_generic_t *th);

void _mpc_threads_ng_engine_handle_zombies(
        _mpc_threads_ng_engine_scheduler_generic_t *th);

int _mpc_threads_ng_engine_attr_destroy(_mpc_threads_ng_engine_attr_t *attr);

typedef enum
{
	MPC_THREADS_GENERIC_BARRIER,
	MPC_THREADS_GENERIC_COND,
	MPC_THREADS_GENERIC_MUTEX,
	MPC_THREADS_GENERIC_RWLOCK,
	MPC_THREADS_GENERIC_SEM,
	MPC_THREADS_GENERIC_TASK_LOCK
}_mpc_threads_ng_engine_lock_list_t;

#endif
