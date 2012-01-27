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
#ifndef __SCTK_THREAD_SCHEDULER_H_
#define __SCTK_THREAD_SCHEDULER_H_

#include <stdlib.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_context.h"
#include <utlist.h>

typedef enum{
  sctk_thread_generic_blocked,
  sctk_thread_generic_running
}sctk_thread_generic_thread_status_t;

typedef struct sctk_thread_generic_scheduler_centralized_s{
  struct sctk_thread_generic_scheduler_centralized_s *prev, *next;
} sctk_thread_generic_scheduler_centralized_t;

typedef struct sctk_thread_generic_scheduler_s{
  sctk_mctx_t ctx;
  union{
    sctk_thread_generic_scheduler_centralized_t centralized;
  };
  sctk_thread_generic_thread_status_t status;
} sctk_thread_generic_scheduler_t;

extern void (*sctk_thread_generic_sched_yield)(sctk_thread_generic_scheduler_t*);
extern void (*sctk_thread_generic_thread_status)(sctk_thread_generic_scheduler_t*,
						sctk_thread_generic_thread_status_t);
extern void (*sctk_thread_generic_register_spinlock_unlock)(sctk_thread_generic_scheduler_t*,
							   sctk_spinlock_t*);
extern void (*sctk_thread_generic_wake)(sctk_thread_generic_scheduler_t*);

struct sctk_thread_generic_p_s;
extern void (*sctk_thread_generic_create)(struct sctk_thread_generic_p_s*);

void sctk_thread_generic_scheduler_init(char* scheduler_type); 
void sctk_thread_generic_scheduler_init_thread(sctk_thread_generic_scheduler_t* sched); 
char* sctk_thread_generic_scheduler_get_name();
#endif
