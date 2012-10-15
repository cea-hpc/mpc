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
#ifdef MPC_Threads
#include "sctk_topology.h"
#include "sctk_thread.h"
#include "sctk_accessor.h"
#endif
#include "sctk_alloc.h"
#include "sctk_debug.h"
#include "sctk_launch.h"

extern int sctk_process_number;
extern int sctk_process_rank;
extern int sctk_node_number;
extern int sctk_node_rank;
extern volatile int sctk_multithreading_initialised;
extern int sctk_migration_mode;

#ifdef MPC_Message_Passing
#include "sctk_inter_thread_comm.h"
/* #include "sctk_low_level_comm.h" */
#endif

#ifdef MPC_Profiler
#include "sctk_internal_profiler.h"
#else
#define SCTK_PROFIL_START(key)	(void)(0)
#define SCTK_PROFIL_END(key) (void)(0)
#define SCTK_COUNTER_INC(key,val) (void)(0);
#define SCTK_COUNTER_DEC(key,val) (void)(0);
#define sctk_internal_profiler_init() (void)(0)
#define sctk_internal_profiler_render() (void)(0)
#define sctk_internal_profiler_release() (void)(0)
#endif
int sctk_user_main (int, char **);
#if 0

#undef main
#define main sctk_user_main
#endif
