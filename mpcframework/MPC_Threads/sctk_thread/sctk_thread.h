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
#ifndef __SCTK_THREADS_H_
#define __SCTK_THREADS_H_

#include "sctk_config.h"
#include <stdlib.h>
#include <signal.h>
#include "sctk_debug.h"
#include "sctk_spinlock.h"
#include "sctk_alloc.h"
#include "sctk_thread_api.h"
#include "sctk_dummy.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SCTK_O * 1
#define SCTK_KO * 1024
#define SCTK_MO * 1024 * 1024

  extern int sctk_is_in_fortran;
#define SCTK_ETHREAD_STACK_SIZE 10 SCTK_MO
#define SCTK_ETHREAD_THREAD_STACK_SIZE 1 SCTK_MO

#ifdef USER_FORTRAN_STACK_SIZE
#define SCTK_ETHREAD_STACK_SIZE_FORTRAN USER_FORTRAN_STACK_SIZE SCTK_MO
#else
#ifdef SCTK_32_BIT_ARCH
#define SCTK_ETHREAD_STACK_SIZE_FORTRAN 12 SCTK_MO
#else
#define SCTK_ETHREAD_STACK_SIZE_FORTRAN 1024 SCTK_MO
#endif
#endif

  int sctk_thread_dump (char *file);
  int sctk_thread_dump_clean (void);
  int sctk_thread_migrate (void);
  int sctk_thread_restore (sctk_thread_t thread, char *type, int vp);
  double sctk_thread_get_activity (int i);


  void sctk_gen_thread_init (void);
  void sctk_pthread_thread_init (void);
  void sctk_ethread_thread_init (void);
  void sctk_ethread_mxn_thread_init (void);
  void sctk_ethread_ng_thread_init (void);
  void sctk_ethread_mxn_ng_thread_init (void);
  void sctk_pthread_ng_thread_init (void);

  void sctk_thread_wait_for_value ( volatile int *data, int value);
  int sctk_thread_timed_wait_for_value (volatile int *data, int value, unsigned int max_time_in_usec);
  void sctk_thread_wait_for_value_and_poll ( volatile int *data, int value,
					    void (*func) (void *), void *arg);
  void
    sctk_kthread_wait_for_value_and_poll ( int *data, int value,
					  void (*func) (void *), void *arg);

  void sctk_thread_freeze_thread_on_vp (sctk_thread_mutex_t * lock,
					void **list);
  void sctk_thread_wake_thread_on_vp (void **list);
  void sctk_thread_init (void);

  int sctk_thread_getattr_np (sctk_thread_t th, sctk_thread_attr_t * attr);
  int sctk_thread_usleep (unsigned int useconds);

  extern volatile int sctk_multithreading_initialised;

#ifndef MPC_Debugger
  typedef enum
  {
    sctk_thread_running_status, sctk_thread_blocked_status,
    sctk_thread_sleep_status, sctk_thread_undef_status,
    sctk_thread_check_status
  } sctk_thread_status_t;
#else
#include <tdb_remote.h>
  typedef td_thr_state_e sctk_thread_status_t;
  #define sctk_thread_running_status TD_THR_ACTIVE
  #define sctk_thread_blocked_status TD_THR_RUN
  #define sctk_thread_sleep_status TD_THR_RUN
  #define sctk_thread_undef_status TD_THR_ZOMBIE
  #define sctk_thread_check_status TD_THR_ACTIVE
#endif

  struct sctk_task_specific_s;
  struct sctk_tls_dtors_s;
  
  typedef struct sctk_thread_data_s
  {
    struct sctk_alloc_chain *tls;
    void *__arg;
    void *(*__start_routine) (void *);
    int task_id;
    int local_task_id;
    int virtual_processor;
    int user_thread;

    volatile struct sctk_thread_data_s *prev;
    volatile struct sctk_thread_data_s *next;
    unsigned long thread_number;
    sctk_thread_t tid;
    volatile sctk_thread_status_t status;
    struct sctk_task_specific_s *father_data;
	struct sctk_tls_dtors_s* dtors_head;
    /* Where the thread must be bound */
    unsigned int bind_to;
  } sctk_thread_data_t;
#define SCTK_THREAD_DATA_INIT { NULL, NULL, NULL, -1, -1, -1 , -1,\
      NULL,NULL,-1,(void*)NULL,sctk_thread_undef_status,NULL}

  void sctk_thread_data_init (void);
  void sctk_thread_data_set (sctk_thread_data_t * task_id);
  sctk_thread_data_t *sctk_thread_data_get (void);

  extern volatile int sctk_thread_running;
  int sctk_thread_get_current_local_tasks_nb();
  void sctk_start_func (void *(*run) (void *), void *arg);
  int sctk_thread_get_vp (void);
  int sctk_get_init_vp_and_nbvp (int i, int *nbVp);
  int sctk_get_init_vp (int i);


  extern volatile unsigned sctk_long_long sctk_timer;

  extern struct sctk_alloc_chain *sctk_thread_tls;

#define sctk_time_interval 10	/*millisecondes */
  int sctk_is_restarted (void);

  int sctk_thread_proc_migration (const int cpu);

  void sctk_thread_init_no_mpc (void);

  sctk_thread_key_t sctk_get_check_point_key (void);
  void sctk_mpc_init_keys (void);
  void sctk_thread_exit_cleanup (void);
  void sctk_ethread_mxn_init_kethread (void);
  void sctk_get_thread_info (int *task_id, int *thread_id);

  int sctk_real_pthread_create (pthread_t  *thread,
    __const pthread_attr_t *attr, void * (*start_routine)(void *), void *arg);

  /* profiling (exec time & dataused) */
  double sctk_profiling_get_init_time();
  double sctk_profiling_get_dataused();
#ifdef __cplusplus
}
#endif
#endif
