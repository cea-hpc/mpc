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

#include "sctk_profile_render_html.h"
#include "sctk_profile_meta.h"

#include <stdlib.h>

#include "mpc_common_debug.h"


void sctk_profile_render_html_register( struct sctk_profile_renderer *rd )
{
	rd->setup = sctk_profile_render_html_setup;
	rd->teardown = sctk_profile_render_html_teardown;

	rd->setup_profile = sctk_profile_render_html_setup_profile;
	rd->render_profile = sctk_profile_render_html_render_profile;
	rd->teardown_profile = sctk_profile_render_html_teardown_profile;

	rd->setup_meta = sctk_profile_render_html_setup_meta;
	rd->render_meta = sctk_profile_render_html_render_meta;
	rd->teardown_meta = sctk_profile_render_html_teardown_meta;
}


void sctk_profile_render_html_setup( struct sctk_profile_renderer *rd )
{
	char output_file[500];

	sctk_profile_render_filename( output_file, "html" );
	rd->output_file = fopen( output_file, "w" );

	if( !rd->output_file )
	{
		mpc_common_debug_error( "Failed to open profile file : %s ", output_file );
		perror( " fopen " );
		abort();
	}

}


void sctk_profile_render_html_teardown( struct sctk_profile_renderer *rd )
{
	fprintf(rd->output_file, "</body>"
							 "</html>");
	fclose( rd->output_file );
	rd->output_file = NULL;
}



void sctk_profile_render_html_setup_meta( struct sctk_profile_renderer *rd )
{


}


void sctk_profile_render_html_render_meta( struct sctk_profile_renderer *rd, struct sctk_profile_meta *meta )
{

	fprintf(rd->output_file, " ");
}


void sctk_profile_render_html_teardown_meta( struct sctk_profile_renderer *rd )
{


}



void sctk_profile_render_html_setup_profile( struct sctk_profile_renderer *rd )
{
	char buff[300];
	fprintf(rd->output_file,
			"<html>\n"
			"<body>\n"
			"<h1>MPC Profile %s</h1>"
			"<table>\n"
			"<tr><td>Name</td><td>Hits</td><td>Total</td><td>Average</td><td>Minimum</td><td>Maximum</td><td>Section</td><td>Global</tr>\n"
			, sctk_profile_renderer_date_clean( buff ));
}



void sctk_profile_render_html_render_profile( struct sctk_profiler_array *array, int id, int parent_id, int depth,  int going_up, struct sctk_profile_renderer *rd )
{
	char buffA[100], buffB[100], buffC[100], buffD[100];

	const char *prefix[3] = { "<B>", " ", "<I>" };
	const char *suffix[3] = { "</B>", " ", "</I>" };

	int prefix_id = (depth < sctk_profile_get_config()->level_colors_size)?depth:sctk_profile_get_config()->level_colors_size - 1;

	char *to_unit_total = NULL;
	char *to_unit_avg = NULL;
	char *to_unit_min = NULL;
	char *to_unit_max = NULL;

	if( sctk_profiler_array_get_type( id ) != SCTK_PROFILE_COUNTER_PROBE  )
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

	char *desc = sctk_profiler_array_get_desc( id );

	if( sctk_profiler_array_get_hits( array, id ) )
	{
		fprintf( rd->output_file,"<tr bgcolor=\"%s\"><td>%s%s%s</td><td>%s%llu%s</td><td>%s%s%s</td><td>%s%s%s</td><td>%s%s%s</td><td>%s%s%s</td><td>%s%g%s</td><td>%s%g%s</td></tr>\n",
																		sctk_profile_get_config()->level_colors[prefix_id], prefix[ (prefix_id < 3)? prefix_id : 2 ], desc, suffix[ (prefix_id < 3)? prefix_id : 2 ],
																		prefix[ (prefix_id < 3)? prefix_id : 2 ], (unsigned long long int )sctk_profiler_array_get_hits( array, id ), suffix[ (prefix_id < 3)? prefix_id : 2 ],
																		prefix[ (prefix_id < 3)? prefix_id : 2 ], to_unit_total,  suffix[ (prefix_id < 3)? prefix_id : 2 ],
																		prefix[ (prefix_id < 3)? prefix_id : 2 ], to_unit_avg,  suffix[ (prefix_id < 3)? prefix_id : 2 ],
																		prefix[ (prefix_id < 3)? prefix_id : 2 ], to_unit_min,  suffix[ (prefix_id < 3)? prefix_id : 2 ],
																		prefix[ (prefix_id < 3)? prefix_id : 2 ], to_unit_max,  suffix[ (prefix_id < 3)? prefix_id : 2 ],
																		prefix[ (prefix_id < 3)? prefix_id : 2 ], rd->ptree.entry_relative_percentage_time[id] * 100,  suffix[ (prefix_id < 3)? prefix_id : 2 ],
																		prefix[ (prefix_id < 3)? prefix_id : 2 ], rd->ptree.entry_total_percentage_time[id] * 100,   suffix[ (prefix_id < 3)? prefix_id : 2 ]);
	}

}


void sctk_profile_render_html_teardown_profile( struct sctk_profile_renderer *rd )
{
	fprintf(rd->output_file, "</table>");
}
