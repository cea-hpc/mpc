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
#ifndef __SCTK_POSIX_PTHREAD_H_
#define __SCTK_POSIX_PTHREAD_H_
#include <stdio.h>

#include "sctk_debug.h"
#include <pthread.h>
#include <semaphore.h>

#include "sctk_posix_pthread.h"
#include "sctk_internal_thread.h"
#include "sctk_posix_ethread_np.h"

#ifdef __cplusplus
extern "C"
{
#endif
void sctk_posix_pthread(void);

#ifdef HAVE_PTHREAD_GETATTR_NP
extern int pthread_getattr_np(pthread_t __th, pthread_attr_t *__attr);
#endif


#ifdef __cplusplus
}
#endif
#endif
