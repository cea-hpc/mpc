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

#include "sctk_debug.h"


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
	char buff[300];
	char output_file[500];
	
	sprintf( output_file, "mpc_profile_%s.html", sctk_profile_renderer_date( buff ) );
	

	rd->output_file = fopen( output_file, "w" );
	
	if( !rd->output_file )
	{
		sctk_error( "Failed to open profile file : %s ", output_file );
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
			"<tr><td>Name</td><td>Hits</td><td>Total Time</td><td>Average Time</td><td>Minimum Time</td><td>Max Time</td><td>Section</td><td>Global</tr>\n"
			, sctk_profile_renderer_date_clean( buff ));
}



void sctk_profile_render_html_render_profile( struct sctk_profiler_array *array, int id, int parent_id, int depth, struct sctk_profile_renderer *rd )
{
	char buffA[500], buffB[500], buffC[500], buffD[500];
	
	const char *prefix[3] = { "<B>", " ", "<I>" };
	const char *suffix[3] = { "</B>", " ", "</I>" };

	const char *colors[3] = { "#3A4D85", "#82A2FF", "#B8BDCB" };


	int prefix_id = (depth<3)?depth:3;

	char *to_unit_total = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_time(array, id) , buffA );
	char *to_unit_avg = sctk_profile_renderer_convert_to_time( rd->ptree.entry_average_time[id] , buffB );
	char *to_unit_min = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_min(array, id) , buffC );
	char *to_unit_max = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_max(array, id) , buffD );

	char *desc = sctk_profiler_array_get_desc( id );

	if( sctk_profiler_array_get_hits( array, id ) )
	{
		fprintf( rd->output_file,"<tr bgcolor=\"%s\"><td>%s%s%s</td><td>%s%llu%s</td><td>%s%s%s</td><td>%s%s%s</td><td>%s%s%s</td><td>%s%s%s</td><td>%s%g%s</td><td>%s%g%s</td></tr>\n",
																		colors[ prefix_id], prefix[ prefix_id ], desc, suffix[ prefix_id ],
																		prefix[ prefix_id ], (unsigned long long int )sctk_profiler_array_get_hits( array, id ), suffix[ prefix_id ],
																		prefix[ prefix_id ], to_unit_total,  suffix[ prefix_id ],
																		prefix[ prefix_id ], to_unit_avg,  suffix[ prefix_id ],
																		prefix[ prefix_id ], to_unit_min,  suffix[ prefix_id ],
																		prefix[ prefix_id ], to_unit_max,  suffix[ prefix_id ],
																		prefix[ prefix_id ], rd->ptree.entry_relative_percentage_time[id] * 100,  suffix[ prefix_id ],
																		prefix[ prefix_id ], rd->ptree.entry_total_percentage_time[id] * 100,   suffix[ prefix_id ]); 
	}

}


void sctk_profile_render_html_teardown_profile( struct sctk_profile_renderer *rd )
{
	fprintf(rd->output_file, "</table>");
}



