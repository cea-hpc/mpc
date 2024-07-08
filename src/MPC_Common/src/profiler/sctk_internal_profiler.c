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

#ifdef MPC_Profiler

#include "sctk_internal_profiler.h"

#include <mpc_common_flags.h>
#include <mpc_common_rank.h>
#include <string.h>

#include "mpc_common_debug.h"
#include "sctk_performance_tree.h"
#include "sctk_profile_render.h"

/* Profiling switch */
int sctk_profiler_internal_switch = 0;
__thread void* mpc_profiler;

/* Profiler Meta */
struct sctk_profile_meta sctk_internal_profiler_meta;

struct sctk_profile_meta *sctk_internal_profiler_get_meta()
{
	return &sctk_internal_profiler_meta;
}


/* Target structure for reduction */
static struct sctk_profiler_array *reduce_array = NULL;

struct sctk_profiler_array * sctk_profiler_get_reduce_array()
{
	int rank = mpc_common_get_task_rank();

	if(!reduce_array && !rank)
	{
		reduce_array = sctk_profiler_array_new();
	}

	return reduce_array;
}

static void sctk_internal_profiler_check_config()
{
	/* Check for at least one color */
	if( sctk_profile_get_config()->level_colors_size < 1 )
	{
		mpc_common_debug_error("You should provide at least one level color to MPC Profiler");
		abort();
	}
}



void sctk_internal_profiler_init()
{
  if (mpc_profiler == NULL) {
    /* Check config options validity */
    sctk_internal_profiler_check_config();

    /* Setup the TLS */
    mpc_profiler = (void *) sctk_profiler_array_new();

    /* Fill in the meta informations */
    sctk_profile_meta_init(&sctk_internal_profiler_meta);

    //	__MPC_Barrier(MPC_COMM_WORLD);

    /* Start Program */
    sctk_profile_meta_begin_compute(&sctk_internal_profiler_meta);

    sctk_profiler_set_initialize_time();
  }
}


/* Defined in sctk_launch.c */

void sctk_internal_profiler_render()
{
	int rank = mpc_common_get_task_rank();

  	sctk_profiler_set_finalize_time();

	/* fprintf(stderr,"%s \n",mpc_common_get_flags()->profiler_outputs); */

	/* End Program */
	sctk_profile_meta_end_compute(&sctk_internal_profiler_meta);

	mpc_common_init_trigger("MPC Profile reduce");

	if( rank == 0 )
	{
		sctk_profiler_array_unify( reduce_array );

		struct sctk_profile_renderer renderer;

		sctk_profile_renderer_init( &renderer, reduce_array, mpc_common_get_flags()->profiler_outputs );

		sctk_profile_renderer_render( &renderer );

		sctk_profile_renderer_release( &renderer );

	}


	/* Free reduce array in rank 0 */
	if( reduce_array && rank == 0 )
	{
		free( reduce_array );
	}

}


void sctk_internal_profiler_release()
{
	sctk_profiler_internal_disable();

	if( mpc_profiler )
		sctk_profiler_array_release(mpc_profiler);

	if( reduce_array  )
		sctk_profiler_array_release(reduce_array);
}



static inline void __set_profiler_output()
{
	char * arg = mpc_common_get_flags()->profiler_outputs;

	if ( strcmp( arg, "undef" ) != 0 )
	{
		if ( sctk_profile_renderer_check_render_list( arg ) )
		{
			mpc_common_debug_error( "Provided profiling output syntax is not correct: %s", arg );
			abort();
		}
	}
}

void mpc_cl_internal_profiler_init() __attribute__((constructor));

void mpc_cl_internal_profiler_init()
{
        MPC_INIT_CALL_ONLY_ONCE

	/* Runtime start */
        mpc_common_init_callback_register("Base Runtime Init Done",
                                          "Init Profiling keys",
                                          __set_profiler_output, 24);

        mpc_common_init_callback_register("Base Runtime Init Done",
                                          "Init Profiling keys",
                                          sctk_internal_profiler_init, 25);

	/* Main start */
        mpc_common_init_callback_register("Before Starting VPs",
                                          "Init Profiling keys",
                                          sctk_profiler_array_init_parent_keys, 24);

        mpc_common_init_callback_register("VP Thread Start",
                                          "Init Profiling keys",
                                          sctk_internal_profiler_init, 24);
}

#endif /* MPC_Profiler */
