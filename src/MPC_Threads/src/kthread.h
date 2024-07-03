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
#ifndef KTHREAD_H_
#define KTHREAD_H_

#include "mpc_threads_config.h"
#include "mpc_common_debug.h"

#include <unistd.h>
#include <signal.h>

#ifdef WINDOWS_SYS
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef SCTK_KERNEL_THREAD_USE_TLS

#define _mpc_thread_kthread_key_create(key, destr_function)    (*(key) ) = NULL
#define _mpc_thread_kthread_key_delete(key)                    (void)(0)

#define _mpc_thread_kthread_setspecific(key, pointer)          ( (key) = (pointer) )
#define _mpc_thread_kthread_getspecific(key)                   (key)

#else

int _mpc_thread_kthread_key_create(mpc_thread_keys_t *key, void (*destr_function)(void *) );
int _mpc_thread_kthread_key_delete(mpc_thread_keys_t key);
int _mpc_thread_kthread_setspecific(mpc_thread_keys_t key, const void *pointer);
void *_mpc_thread_kthread_getspecific(mpc_thread_keys_t key);
#endif

typedef mpc_thread_t   mpc_thread_kthread_t;

int mpc_thread_kthread_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                                      void *(*start_routine)(void *), void *arg);
int _mpc_thread_kthread_create(mpc_thread_kthread_t *thread, void *(*start_routine)(void *), void *arg);
int _mpc_thread_kthread_join(mpc_thread_kthread_t th, void **thread_return);
int _mpc_thread_kthread_kill(mpc_thread_kthread_t th, int val);
mpc_thread_kthread_t _mpc_thread_kthread_self(void);
void _mpc_thread_kthread_exit(void *retval);


int _mpc_thread_kthread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask);

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static inline void kthread_usleep(unsigned long usec)
{
#ifndef WINDOWS_SYS
	usleep(usec);
#else
	Sleep(usec);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* KTHREAD_H_ */
