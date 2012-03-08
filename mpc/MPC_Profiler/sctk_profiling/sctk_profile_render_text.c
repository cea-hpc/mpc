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

#include "sctk_profile_render_text.h"

#include <stdlib.h>

#include "sctk_debug.h"

static int sctk_profile_render_text_is_stdout = 0;

void sctk_profile_render_text_register( struct sctk_profile_renderer *rd , int is_stdout)
{
	rd->setup = sctk_profile_render_text_setup;
	rd->teardown = sctk_profile_render_text_teardown;
	rd->render_entry = sctk_profile_render_text_render_entry;
	
	sctk_profile_render_text_is_stdout = is_stdout;
}


void sctk_profile_render_text_setup( struct sctk_profile_renderer *rd )
{
	char buff[300];
	char output_file[500];
	
	sprintf( output_file, "mpc_profile_%s.txt", sctk_profile_renderer_date( buff ) );
	
	if( !sctk_profile_render_text_is_stdout )
	{
	
		rd->output_file = fopen( output_file, "w" );
		
		if( !rd->output_file )
		{
			sctk_error( "Failed to open profile file : %s ", output_file );
			perror( " fopen " );
			abort();
		}
		
	}
	else
	{
		rd->output_file = stdout;
	}
	

}


void sctk_profile_render_text_teardown( struct sctk_profile_renderer *rd )
{
	if( !sctk_profile_render_text_is_stdout )
	{
		fclose( rd->output_file );
	}
	
	rd->output_file = NULL;
	
}


void sctk_profile_render_text_render_entry( struct sctk_profiler_array *array, int id, int parent_id, int depth, struct sctk_profile_renderer *rd )
{
	char buff[500];
	char *to_unit = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_time(array, id) , buff );

	if( sctk_profiler_array_get_hits( array, id ) )
	{
		sctk_profile_renderer_write_ntabs( rd->output_file, depth );
		fprintf( rd->output_file, "%s %lld Hits %s ( %g\% )\n",  sctk_profiler_array_get_desc( id ), sctk_profiler_array_get_hits( array, id ), to_unit, rd->ptree.entry_total_percentage_time[id] * 100);
	}

}




