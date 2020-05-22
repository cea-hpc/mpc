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
#include "mpc_common_helper.h"
#include <stdlib.h>

#include "mpc_common_debug.h"

static int sctk_profile_render_text_is_stdout = 0;
static int sctk_profile_render_text_is_raw = 0;
static int sctk_profile_render_text_no_indent = 0;


void sctk_profile_render_text_register( struct sctk_profile_renderer *rd , int is_stdout, int is_raw, int no_indent)
{
	rd->setup = sctk_profile_render_text_setup;
	rd->teardown = sctk_profile_render_text_teardown;
	rd->render_profile = sctk_profile_render_text_render_entry;

	rd->setup_meta = sctk_profile_render_text_setup_meta;
	rd->render_meta = sctk_profile_render_text_render_meta;
	rd->teardown_meta = sctk_profile_render_text_teardown_meta;

	sctk_profile_render_text_is_stdout = is_stdout;
	sctk_profile_render_text_is_raw = is_raw;
	sctk_profile_render_text_no_indent = no_indent;
}


void sctk_profile_render_text_setup_meta( struct sctk_profile_renderer *rd )
{


}

void sctk_profile_render_text_teardown_meta( struct sctk_profile_renderer *rd )
{


}

void sctk_profile_render_text_render_meta( struct sctk_profile_renderer *rd, struct sctk_profile_meta *meta )
{



}


void sctk_profile_render_text_setup( struct sctk_profile_renderer *rd )
{
	char output_file[500];

	if( !sctk_profile_render_text_is_stdout )
	{
		sctk_profile_render_filename( output_file, "txt" );

		rd->output_file = fopen( output_file, "w" );

		if( !rd->output_file )
		{
			mpc_common_debug_error( "Failed to open profile file : %s ", output_file );
			perror( " fopen " );
			abort();
		}

	}
	else
	{
		rd->output_file = stdout;
	}

	fprintf(rd->output_file, "#NAME HITS TOTAL TOTAL_PCT AVG MIN MAX\n\n");


}


void sctk_profile_render_text_teardown( struct sctk_profile_renderer *rd )
{
	if( !sctk_profile_render_text_is_stdout )
	{
		fclose( rd->output_file );
	}

	rd->output_file = NULL;

}


void sctk_profile_render_text_render_entry( struct sctk_profiler_array *array, int id, int parent_id, int depth,  int going_up, struct sctk_profile_renderer *rd )
{
	char buffA[100], buffB[100], buffC[100], buffD[100];

	char *to_unit_total = NULL;
	char *to_unit_avg = NULL;
	char *to_unit_min = NULL;
	char *to_unit_max = NULL;

	if( !sctk_profile_render_text_is_raw  && sctk_profiler_array_get_type( id ) != SCTK_PROFILE_COUNTER_PROBE  )
	{
		if( sctk_profiler_array_get_type( id ) == SCTK_PROFILE_COUNTER_SIZE_PROBE)
		{
			to_unit_total = sctk_profile_renderer_convert_to_size( sctk_profiler_array_get_value(array, id) , buffA );
			to_unit_avg = sctk_profile_renderer_convert_to_size( rd->ptree.entry_average_time[id] , buffB );
			to_unit_min = sctk_profile_renderer_convert_to_size( sctk_profiler_array_get_min(array, id) , buffC );
			to_unit_max = sctk_profile_renderer_convert_to_size( sctk_profiler_array_get_max(array, id) , buffD );
		}
		else if( sctk_profiler_array_get_type( id ) == SCTK_PROFILE_TIME_PROBE )
		{
			to_unit_total = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_value(array, id) , buffA );
			to_unit_avg = sctk_profile_renderer_convert_to_time( rd->ptree.entry_average_time[id] , buffB );
			to_unit_min = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_min(array, id) , buffC );
			to_unit_max = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_max(array, id) , buffD );
		}
	}
	else
	{
		sprintf(buffA, "%llu", (unsigned long long int )sctk_profiler_array_get_value(array, id));
		to_unit_total = buffA;

		sprintf(buffB, "%llu", (unsigned long long int )rd->ptree.entry_average_time[id]);
		to_unit_avg = buffB;

		sprintf(buffC, "%llu", (unsigned long long int )sctk_profiler_array_get_min(array, id));
		to_unit_min = buffC;

		sprintf(buffD, "%llu", (unsigned long long int )sctk_profiler_array_get_max(array, id));
		to_unit_max = buffD;
	}

	if( sctk_profiler_array_get_hits( array, id ) )
	{

		if( !sctk_profile_render_text_no_indent )
		{
			printf("|");
			sctk_profile_renderer_write_ntabs( rd->output_file, depth );
		}

		if( sctk_profile_get_config()->color_stdout && sctk_profile_render_text_is_stdout )
		{
				fprintf( rd->output_file, MPC_COLOR_RED_BOLD(%-15s)"  "MPC_COLOR_BLUE_BOLD(%10llu)"  "MPC_COLOR_GREEN_BOLD(%-10s)"  "
										  MPC_COLOR_VIOLET_BOLD(( %.2f %% ))"  %-10s  %-10s  %-10s\n", sctk_profiler_array_get_desc( id ), (unsigned long long int )sctk_profiler_array_get_hits( array, id ),
																			 to_unit_total, rd->ptree.entry_total_percentage_time[id] * 100,
																			 to_unit_avg, to_unit_min, to_unit_max);
		}
		else
		{
			fprintf( rd->output_file, "%-15s  %10llu  %-10s  ( %.2f %% )  %-10s  %-10s  %-10s\n", sctk_profiler_array_get_desc( id ), (unsigned long long int )sctk_profiler_array_get_hits( array, id ),
																		 to_unit_total, rd->ptree.entry_total_percentage_time[id] * 100,
																		 to_unit_avg, to_unit_min, to_unit_max);
		}
	}

}




