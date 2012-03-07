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
#include "sctk_internal_profiler.h"

#include "sctk_debug.h"
#include "mpc_mpi.h"

/* Profiling switch */
int sctk_profiler_internal_switch = 0;

/* Target structure for reduction */
static struct sctk_profiler_array *reduce_array = NULL;


void sctk_internal_profiler_init()
{
	printf("TLS prof init \n");
	/* Setup the TLS */
	tls_mpc_profiler = (void *) sctk_profiler_array_new();

	sctk_profiler_internal_enable();
}



void sctk_internal_profiler_reduce()
{
	if( !reduce_array )
	{
		reduce_array = sctk_profiler_array_new();
	}

	if( !sctk_internal_profiler_get_tls_array())
	{
		sctk_error("Profiler TLS is not initialized");
		return;
	}

	if( sctk_profiler_internal_enabled() )
	{
		sctk_error( "This section must be entered with a disbled profiler");
		abort();
	}


	PMPC_Reduce( sctk_internal_profiler_get_tls_array()->sctk_profile_hits,
				 reduce_array->sctk_profile_hits,
		         SCTK_PROFILE_KEY_COUNT,
		         MPC_LONG_LONG_INT,
		         MPC_SUM,
		         0,
		         MPC_COMM_WORLD);

	PMPC_Reduce( sctk_internal_profiler_get_tls_array()->sctk_profile_time,
	             reduce_array->sctk_profile_time,
		         SCTK_PROFILE_KEY_COUNT,
		         MPC_LONG_LONG_INT,
		         MPC_SUM,
		         0,
		         MPC_COMM_WORLD);
	
	PMPC_Reduce( sctk_internal_profiler_get_tls_array()->sctk_profile_max,
	             reduce_array->sctk_profile_max,
		         SCTK_PROFILE_KEY_COUNT,
		         MPC_LONG_LONG_INT,
		         MPC_MAX,
		         0,
		         MPC_COMM_WORLD);
		     
	PMPC_Reduce( sctk_internal_profiler_get_tls_array()->sctk_profile_min,
				 reduce_array->sctk_profile_min,
		         SCTK_PROFILE_KEY_COUNT,
		         MPC_LONG_LONG_INT,
		         MPC_MIN,
		         0,
		         MPC_COMM_WORLD);


	free( reduce_array );
}




void sctk_internal_profiler_render()
{
	sctk_profiler_internal_disable();

	sctk_internal_profiler_reduce();



}



void sctk_internal_profiler_release()
{
	sctk_profiler_internal_disable();

	if( tls_mpc_profiler )
		sctk_profiler_array_release(tls_mpc_profiler);

	if( reduce_array  )
		sctk_profiler_array_release(reduce_array);
}
