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
#ifndef __SCTK_POSIX_ETHREAD_NP_H_
#define __SCTK_POSIX_ETHREAD_NP_H_
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_ethread_internal.h"
#include "mpc_common_spinlock.h"
#include "sctk_asm.h"
#include "sctk_pthread_compatible_structures.h"
#include "sctk_ethread.h"

#ifdef __cplusplus
extern "C"
{
#endif
  static inline int __sctk_ethread_getattr_np (sctk_ethread_t th,
					       sctk_ethread_attr_intern_t
					       * attr)
  {
    sctk_nodebug ("getattr : 1 :  %p , %p", th, attr);
    *attr = th->attr;
    sctk_nodebug ("sortie de getattr");
    return 0;
  }
#ifdef __cplusplus
}
#endif
#endif
