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

/*    _
     / \
    / | \        **************************************
   /  |  \       |  Structures MUST multiple of int.  |
  /       \      **************************************
 /    *    \   
/___________\

*/

#ifdef __cplusplus
extern "C"
{
#endif

  typedef volatile int sctk_spinlock_t;
#define SCTK_SPINLOCK_INITIALIZER 0

  /*Threads definitions */
  struct sctk_ethread_per_thread_s;
  typedef struct sctk_ethread_per_thread_s *sctk_ethread_t;

  struct sctk_ethread_attr_intern_s;
  typedef struct
  {
    struct sctk_ethread_attr_intern_s *ptr;
  } sctk_ethread_attr_t;

  /*Condition definition */
  struct sctk_ethread_mutex_cell_s;
  typedef struct
  {
    sctk_spinlock_t lock;
    volatile int is_init;
    volatile struct sctk_ethread_mutex_cell_s *list;
    volatile struct sctk_ethread_mutex_cell_s *list_tail;
  } sctk_ethread_cond_t;
#define SCTK_ETHREAD_COND_INIT {SCTK_SPINLOCK_INITIALIZER,0,NULL,NULL}

  /*condition attributes */
  typedef struct
  {
    short pshared;
    short clock;
  } sctk_ethread_condattr_t;

  /*Semaphore management */
  typedef struct
  {
    volatile int lock;
    sctk_spinlock_t spinlock;
    volatile struct sctk_ethread_mutex_cell_s *list;
    volatile struct sctk_ethread_mutex_cell_s *list_tail;
  } sctk_ethread_sem_t;
#define STCK_ETHREAD_SEM_INIT	{0,SCTK_SPINLOCK_INITIALIZER,NULL,NULL}

  /*Mutex definition */
  struct sctk_ethread_mutex_cell_s;

  typedef struct
  {
    volatile struct sctk_ethread_per_thread_s *owner;
    volatile struct sctk_ethread_mutex_cell_s *list;
    volatile struct sctk_ethread_mutex_cell_s *list_tail;
    sctk_spinlock_t spinlock;
    volatile unsigned int lock;
    unsigned int type;
  } sctk_ethread_mutex_t;
#define SCTK_ETHREAD_MUTEX_INIT	{NULL,NULL,NULL,SCTK_SPINLOCK_INITIALIZER,0,SCTK_THREAD_MUTEX_DEFAULT}

  typedef struct
  {
    unsigned short kind;
  } sctk_ethread_mutexattr_t;

/*Rwlock*/
  typedef struct sctk_ethread_rwlock_cell_s
  {
    struct sctk_ethread_rwlock_cell_s *next;
    struct sctk_ethread_per_thread_s *my_self;
    volatile int wake;
    volatile unsigned short type;
  } sctk_ethread_rwlock_cell_t;


  typedef struct
  {
    sctk_spinlock_t spinlock;
    volatile sctk_ethread_rwlock_cell_t *list;
    volatile sctk_ethread_rwlock_cell_t *list_tail;
    volatile unsigned int lock;
    volatile unsigned short current;
    volatile unsigned short wait;
  } sctk_ethread_rwlock_t;
#define SCTK_ETHREAD_RWLOCK_INIT	{SCTK_SPINLOCK_INITIALIZER,NULL,NULL,0,SCTK_RWLOCK_ALONE,SCTK_RWLOCK_NO_WR_WAIT}
#define SCTK_RWLOCK_READ 1
#define SCTK_RWLOCK_WRITE 2
#define SCTK_RWLOCK_ALONE 0
#define SCTK_RWLOCK_NO_WR_WAIT 0
#define SCTK_RWLOCK_WR_WAIT 1

/*RwLock attributes*/
  typedef struct
  {
    int pshared;
  } sctk_ethread_rwlockattr_t;


/*barrier*/
  typedef struct
  {
    sctk_spinlock_t spinlock;
    volatile struct sctk_ethread_mutex_cell_s *list;
    volatile struct sctk_ethread_mutex_cell_s *list_tail;
    volatile unsigned int nb_max;
    volatile unsigned int lock;
  } sctk_ethread_barrier_t;

/*barrier attributes*/
  typedef sctk_ethread_rwlockattr_t sctk_ethread_barrierattr_t;
#ifdef __cplusplus
}
#endif
#endif
