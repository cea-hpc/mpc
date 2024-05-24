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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef SCTK_INTERNAL_PROFILER
#define SCTK_INTERNAL_PROFILER

#include <mpc_common_types.h>

#ifdef MPC_Profiler

#include <mpc_common_profiler.h>
#include "mpc_common_debug.h"
#include "sctk_profiler_array.h"
#include "sctk_profile_meta.h"
#include "mpc_common_asm.h"


struct sctk_profile_meta *sctk_internal_profiler_get_meta();

/* ****************** */

/* ****************** */

/* MPI Init / MPI Finalize */
static inline void sctk_profiler_set_initialize_time()
{
	struct sctk_profiler_array* array = (struct sctk_profiler_array *)mpc_profiler;
	array->initialize_time = mpc_arch_get_timestamp();
	sctk_profiler_internal_enable();
}

static inline void sctk_profiler_set_finalize_time()
{
	struct sctk_profiler_array* array = (struct sctk_profiler_array *)mpc_profiler;
	sctk_profiler_internal_disable();
  	array->run_time = ( (double)mpc_arch_get_timestamp() - (double)array->initialize_time);
}

#endif /* MPC_Profiler */

#endif /* SCTK_INTERNAL_PROFILER */
