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
#ifndef __SCTK_KTHREAD_H_
#define __SCTK_KTHREAD_H_

#include "mpcthread_config.h"
#include "sctk_debug.h"

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

#define kthread_key_create(key, destr_function)    (*(key) ) = NULL
#define kthread_key_delete(key)                    (void)(0)

#define kthread_setspecific(key, pointer)          ( (key) = (pointer) )
#define kthread_getspecific(key)                   (key)

#else
typedef sctk_thread_key_t   kthread_key_t;

int kthread_key_create(kthread_key_t *key,
                       void (*destr_function)(void *) );
int kthread_key_delete(kthread_key_t key);
int kthread_setspecific(kthread_key_t key, const void *pointer);
void *kthread_getspecific(kthread_key_t key);
#endif

typedef sctk_thread_t   kthread_t;

int kthread_create(kthread_t *thread, void *(*start_routine)(void *),
                   void *arg);
int kthread_join(kthread_t th, void **thread_return);
int kthread_kill(kthread_t th, int val);
kthread_t kthread_self(void);
void kthread_exit(void *retval);


int kthread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask);

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
#endif
