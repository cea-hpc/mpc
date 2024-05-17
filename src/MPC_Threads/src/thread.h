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
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_THREADS_H_
#define __SCTK_THREADS_H_


#include <stdlib.h>
#include <signal.h>

#include <mpc_common_debug.h>
#include <mpc_thread_accessor.h>


#include "mpc_common_spinlock.h"
#include "sctk_alloc.h"
#include "mpc_thread.h"

#include "thread_dbg.h"

#include "sctk_dummy.h"

#ifdef MPC_Lowcomm
#include "mpc_lowcomm_workshare.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/************************
 * THREAD MODULE CONFIG *
 ************************/

struct mpc_thread_config
{
	/* Common */
	char thread_layout[MPC_CONF_STRING_SIZE];

    /* enable/disable timer kernel thread, that counts tick */
    int thread_timer_enabled;

    int thread_timer_interval;

	/* Kthread */
	long int kthread_stack_size;

	/* Ethread */
	long int ethread_spin_delay;
};

struct mpc_thread_config  * _mpc_thread_config_get(void);


int mpc_thread_dump(char *file);
int mpc_thread_dump_clean(void);
int mpc_thread_migrate(void);
int mpc_thread_dump_restore(mpc_thread_t thread, char *type, int vp);


int mpc_thread_getattr_np(mpc_thread_t th, mpc_thread_attr_t *attr);
int mpc_thread_usleep(unsigned int useconds);

extern volatile int sctk_multithreading_initialised;

#ifndef MPC_Thread_db
typedef enum
{
	sctk_thread_running_status, sctk_thread_blocked_status,
	mpc_thread_sleep_status, sctk_thread_undef_status,
	sctk_thread_check_status
} sctk_thread_status_t;
#else
#include <tdb_remote.h>
typedef td_thr_state_e   sctk_thread_status_t;
#define sctk_thread_running_status    TD_THR_ACTIVE
#define sctk_thread_blocked_status    TD_THR_RUN
#define mpc_thread_sleep_status       TD_THR_RUN
#define sctk_thread_undef_status      TD_THR_ZOMBIE
#define sctk_thread_check_status      TD_THR_ACTIVE
#endif

struct mpc_mpi_cl_per_mpi_process_ctx_s;
struct sctk_tls_dtors_s;


typedef struct sctk_thread_data_s
{
	struct sctk_alloc_chain *                tls;
	void *                                   __arg;
	void *(*__start_routine)(void *);
	mpc_thread_rank_info_t mpi_task;
	int                                      virtual_processor;
	int                                      user_thread;

	volatile struct sctk_thread_data_s *     prev;
	volatile struct sctk_thread_data_s *     next;
	unsigned long                            thread_number;
	mpc_thread_t                             tid;
	volatile sctk_thread_status_t            status;
	struct mpc_mpi_cl_per_mpi_process_ctx_s *mpc_mpi_context_data;
	struct sctk_tls_dtors_s *                dtors_head;
	/* Where the thread must be bound */
	unsigned int                             bind_to;
  /* This is the MPI interface per th ctx */
  void *                                   mpi_per_thread;
  /* The thread disguisement if present */
  mpc_thread_mpi_disguise_t disguise;
#ifdef MPC_Lowcomm
#ifdef MPC_ENABLE_WORKSHARE
  mpc_workshare *                          workshare;
#endif
  int                                      is_in_collective;
#endif

} sctk_thread_data_t;

#define SCTK_THREAD_DATA_INIT    { NULL, NULL, NULL, {-1, -1}, -1, -1, NULL,           \
		                   NULL, -1, (void *)NULL, sctk_thread_undef_status, \
		                   NULL, NULL, -1, NULL, {NULL, NULL},0,0 }

void _mpc_thread_data_init(void);
void _mpc_thread_data_set(sctk_thread_data_t *task_id);

sctk_thread_data_t *mpc_thread_data_get_disg(int no_disguise);



extern volatile unsigned sctk_long_long ___timer_thread_ticks;
extern volatile int ___timer_thread_running;

extern struct sctk_alloc_chain *mpc_thread_tls;

void _mpc_thread_exit_cleanup(void);
void _mpc_thread_ethread_mxn_engine_init_kethread(void);

int sctk_real_pthread_create(pthread_t *thread,
                             __const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);

/* At exit */
int mpc_thread_atexit(void (*function)(void) );

/* profiling (exec time & dataused) */
double sctk_profiling_get_init_time();
double sctk_profiling_get_dataused();

#ifdef __cplusplus
}
#endif
#endif
