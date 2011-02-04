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
#ifndef __SCTK__ALLOC_THREAD
#define __SCTK__ALLOC_THREAD

#ifdef __cplusplus
extern "C"
{
#endif

#include "sctk_config.h"
#include "sctk_spinlock.h"

#ifdef MPC_Threads
#include "sctk_context.h"

#define sctk_once sctk_thread_once
#define sctk_once_t sctk_thread_once_t
#define SCTK_ONCE_INIT SCTK_THREAD_ONCE_INIT

#define sctk_alloc_spinlock_t sctk_spinlock_t
#define sctk_alloc_spinlock_lock sctk_spinlock_lock
#define sctk_alloc_spinlock_unlock sctk_spinlock_unlock
#define SCTK_ALLOC_SPINLOCK_INITIALIZER SCTK_SPINLOCK_INITIALIZER

#define sctk_mutex_t sctk_thread_mutex_t
#define sctk_mutex_lock sctk_thread_lock_lock
#define sctk_mutex_unlock sctk_thread_lock_unlock
#define SCTK_MUTEX_INITIALIZER SCTK_THREAD_MUTEX_INITIALIZER

#ifdef __SCTK__ALLOC__C__

  extern volatile int sctk_multithreading_initialised;
  void *sctk_thread_getspecific (sctk_thread_key_t __key);
  int sctk_thread_setspecific (sctk_thread_key_t __key,
			       const void *__pointer);
  int sctk_thread_key_create (sctk_thread_key_t * __key,
			      void (*__destr_function) (void *));
  int sctk_thread_mutex_lock (sctk_thread_mutex_t * __mutex);
  int sctk_thread_mutex_spinlock (sctk_thread_mutex_t * __mutex);
  int sctk_thread_mutex_unlock (sctk_thread_mutex_t * __mutex);


#ifdef SCTK_USE_TLS
  __thread void *sctk_tls_key;
#else
  static sctk_thread_key_t sctk_tls_key;
#endif

  static void *sctk_tls_default = NULL;

  static mpc_inline void *sctk_get_tls_from_thread ()
  {
    if (sctk_multithreading_initialised == 0)
      return sctk_tls_default;
    else
      {
#ifdef SCTK_USE_TLS
	return sctk_tls_key;
#else
	return sctk_thread_getspecific (sctk_tls_key);
#endif
      }
  }

  static mpc_inline void *sctk_get_tls_from_thread_no_check ()
  {
    if (sctk_multithreading_initialised == 0)
      return sctk_tls_default;
    else
      {
#ifdef SCTK_USE_TLS
	return sctk_tls_key;
#else
	return sctk_thread_getspecific (sctk_tls_key);
#endif
      }
  }

  static mpc_inline void sctk_set_tls_from_thread (void *ptr)
  {
    if (sctk_multithreading_initialised == 0)
      sctk_tls_default = ptr;
    else
      {
#ifdef SCTK_USE_TLS
	sctk_tls_key = ptr;
#else
	sctk_thread_setspecific (sctk_tls_key, ptr);
#endif
      }
  }

  static mpc_inline void sctk_init_tls_from_thread ()
  {
    if (sctk_multithreading_initialised == 0)
      return;
    else
      {
#ifndef SCTK_USE_TLS
	sctk_thread_key_create (&sctk_tls_key, NULL);
#endif
      }
  }

  static mpc_inline void sctk_thread_lock_lock (sctk_mutex_t * mut)
  {
    if (sctk_multithreading_initialised == 1)
      sctk_thread_mutex_spinlock (mut);
  }

  static mpc_inline void sctk_thread_lock_unlock (sctk_mutex_t * mut)
  {
    if (sctk_multithreading_initialised == 1)
      sctk_thread_mutex_unlock (mut);
  }

#endif

#else				/* SCTK_ALLOC_EXTERN */
#define PTHREAD_USE_TLS
   /*PTHREAD*/
#include <pthread.h>

#define sctk_once pthread_once
#define sctk_once_t pthread_once_t
#define SCTK_ONCE_INIT PTHREAD_ONCE_INIT
#define sctk_mutex_t pthread_mutex_t
#define sctk_mutex_lock pthread_mutex_lock
#define sctk_mutex_unlock pthread_mutex_unlock
#define SCTK_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define sctk_alloc_spinlock_t pthread_mutex_t
#define sctk_alloc_spinlock_lock pthread_mutex_lock
#define sctk_alloc_spinlock_unlock pthread_mutex_unlock
#define SCTK_ALLOC_SPINLOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define sctk_thread_self pthread_self
#define sctk_thread_t pthread_t
#ifdef __SCTK__ALLOC__C__
#ifdef PTHREAD_USE_TLS
  static __thread void *sctk_tls_key = NULL;
#define sctk_get_tls_from_thread_no_check() sctk_tls_key
#define sctk_get_tls_from_thread() sctk_tls_key
#define sctk_init_tls_from_thread() sctk_tls_key = NULL
#define sctk_set_tls_from_thread(a) sctk_tls_key = a
#else
  static pthread_key_t sctk_tls_key;
  static mpc_inline void *sctk_get_tls_from_thread_no_check ()
  {
    return pthread_getspecific (sctk_tls_key);
  }
  static mpc_inline void *sctk_get_tls_from_thread ()
  {
    return pthread_getspecific (sctk_tls_key);
  }
  static mpc_inline void sctk_init_tls_from_thread ()
  {
    pthread_key_create (&sctk_tls_key, NULL);
  }
  static mpc_inline void sctk_set_tls_from_thread (void *ptr)
  {
    pthread_setspecific (sctk_tls_key, ptr);
  }
#endif
#endif
#endif

#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
