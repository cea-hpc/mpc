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
/*
    File generated automaticely by /home/users/aserraz/framework/mpc//configure
*/
#ifndef __SCTK_CONFIG_H_
#define __SCTK_CONFIG_H_
#ifdef __SCTK__INTERNALS__
#include "sctk_keywords.h"
#endif


#include <sctk_config_arch.h>

#if 0
/* Configuration: */
#define Linux_SYS
#define SCTK_x86_64_ARCH_SCTK

#ifdef SCTK_i386_ARCH_SCTK
#define SCTK_i686_ARCH_SCTK
#endif

#ifdef SCTK_i486_ARCH_SCTK
#define SCTK_i686_ARCH_SCTK
#endif

#ifdef SCTK_i586_ARCH_SCTK
#define SCTK_i686_ARCH_SCTK
#endif

#define SCTK_OS Linux_SYS
#define SCTK_ARCH_SCTK SCTK_x86_64_ARCH_SCTK
#endif

/*BEGIN GENRERATION*/

/* Pthread compatibility: */
/*We use int in order to deal with data alignment*/
typedef void* sctk_thread_t;

/* Pthread mutex compatibility: */
/*Real size 40 of pthread_mutex_t*/
typedef struct {long word_long[5];} sctk_thread_mutex_t;
#define SCTK_THREAD_MUTEX_INITIALIZER {{0,0,0,0,0}}
/*Real size 4 of pthread_mutexattr_t*/
typedef int sctk_thread_mutexattr_t;
#define sctk_thread_mutexattr_t_is_contiguous
#define sctk_thread_mutexattr_t_is_contiguous_int
#define SCTK_THREAD_MUTEX_NORMAL 0
#define SCTK_THREAD_MUTEX_RECURSIVE 1
#define SCTK_THREAD_MUTEX_ERRORCHECK 2
#define SCTK_THREAD_MUTEX_DEFAULT 0

/* Pthread semaphores compatibility: */
/*Real size 32 of sem_t*/
typedef struct {long word_long[4];} sctk_thread_sem_t;
#define SCTK_SEM_FAILED 0
#define SCTK_SEM_VALUE_MAX 2147483647

/* Pthread conditions compatibility: */
/*Real size 4 of pthread_condattr_t*/
typedef int sctk_thread_condattr_t;
#define sctk_thread_condattr_t_is_contiguous
#define sctk_thread_condattr_t_is_contiguous_int
/*Real size 48 of pthread_cond_t*/
typedef struct {long word_long[6];} sctk_thread_cond_t;
#define SCTK_THREAD_COND_INITIALIZER {{0,0,0,0,0,0}}

/* Pthread barrier compatibility: */
/*Real size 4 of pthread_barrierattr_t*/
typedef int sctk_thread_barrierattr_t;
#define sctk_thread_barrierattr_t_is_contiguous
#define sctk_thread_barrierattr_t_is_contiguous_int
/*Real size 32 of pthread_barrier_t*/
typedef struct {long word_long[4];} sctk_thread_barrier_t;
#define SCTK_THREAD_BARRIER_SERIAL_THREAD -1

/* Pthread spinlock compatibility: */
/*Real size 4 of pthread_spinlock_t*/
typedef int sctk_thread_spinlock_t;
#define sctk_thread_spinlock_t_is_contiguous
#define sctk_thread_spinlock_t_is_contiguous_int

/* Pthread rwlock compatibility: */
/*Real size 8 of pthread_rwlockattr_t*/
typedef long sctk_thread_rwlockattr_t;
#define sctk_thread_rwlockattr_t_is_contiguous
#define sctk_thread_rwlockattr_t_is_contiguous_long
/*Real size 56 of pthread_rwlock_t*/
typedef struct {long word_long[7];} sctk_thread_rwlock_t;
#define SCTK_THREAD_RWLOCK_INITIALIZER {{0,0,0,0,0,0,0}}

/* Pthread keys compatibility: */
#define SCTK_THREAD_KEYS_MAX 1024
/*Real size 4 of pthread_key_t*/
typedef int sctk_thread_key_t;
#define sctk_thread_key_t_is_contiguous
#define sctk_thread_key_t_is_contiguous_int

/* Pthread once compatibility: */
/*Real size 4 of pthread_once_t*/
typedef int sctk_thread_once_t;
#define sctk_thread_once_t_is_contiguous
#define sctk_thread_once_t_is_contiguous_int
#define SCTK_THREAD_ONCE_INIT 0

/* Pthread attr compatibility: */
/*Real size 56 of pthread_attr_t*/
typedef struct {long word_long[7];} sctk_thread_attr_t;
#define SCTK_THREAD_CREATE_JOINABLE 0
#define SCTK_THREAD_CREATE_DETACHED 1
#define SCTK_THREAD_INHERIT_SCHED 0
#define SCTK_THREAD_EXPLICIT_SCHED 1
#define SCTK_THREAD_SCOPE_SYSTEM 0
#define SCTK_THREAD_SCOPE_PROCESS 1
#define SCTK_THREAD_PROCESS_PRIVATE 0
#define SCTK_THREAD_PROCESS_SHARED 1
#define SCTK_THREAD_CANCEL_ENABLE 0
#define SCTK_THREAD_CANCEL_DISABLE 1
#define SCTK_THREAD_CANCEL_DEFERRED 0
#define SCTK_THREAD_CANCEL_ASYNCHRONOUS 1
#define SCTK_THREAD_CANCELED (void*)-1
#define SCTK_SCHED_OTHER 0
#define SCTK_SCHED_RR 2
#define SCTK_SCHED_FIFO 1
#define SCTK_O_CREAT 64
#define SCTK_O_EXCL 128
#define SCTK_THREAD_STACK_MIN 16384

/* Error values: */
#define SCTK_ESRCH 3
#define SCTK_EINVAL 22
#define SCTK_EDEADLK 35
#define SCTK_ENOTSUP 95
#define SCTK_EPERM 1
#define SCTK_EBUSY 16
#define SCTK_EAGAIN 11
#define SCTK_ESRCH 3
#define SCTK_ENOSPC 28
#define SCTK_ENOENT 2
#define SCTK_EINTR 4
#define SCTK_ETIMEDOUT 110
#define SCTK_ENOSYS 38
#define SCTK_EOVERFLOW 75
#define SCTK_ENAMETOOLONG 36
#define SCTK_EEXIST 17
#define SCTK_EACCES 13
#define SCTK_ENOMEM 12

/* Memory: */
#define SCTK_MAX_MEMORY_SIZE ((unsigned long)52776558132224)
#define SCTK_MAX_MEMORY_OFFSET ((unsigned long)2147483648)
#define SCTK_PAGE_SIZE ((unsigned long)4096)
#define SCTK_NSIG 65
/*END GENRERATION*/
#define HAVE_UTSNAME

/* Options: */
#define SCTK_VERSION_MAJOR 2
#define SCTK_VERSION_MINOR 4
#define SCTK_VERSION_REVISION 1
#define SCTK_VERSION_PRE ""

/*  Install prefix */
#define SCTK_INSTALL_PREFIX @SCTK_PREFIX@
#endif
