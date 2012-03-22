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

#include "sctk_profile_render_tex.h"

#include <stdlib.h>
#include <string.h>
#include "sctk_debug.h"


char *sctk_profile_render_tex_sanitize_string( char *string )
{
  int len = strlen( string );
  char *ret = malloc( 2* len );
  
  int i = 0;
  int off = 0;
  
  for( i = 0 ; i <= len ; i++)
  {
    if( i == len )
      ret[off] = '\0';
    
    if( off == len * 2 )
    {
	ret[off]= '\0';
    }
    
    
    if( string[i] == '_' )
    {
	ret[off] = '\\';
	off++;
	ret[off] = '_';
    }
    else if( string[i] == '&' )
    {
	ret[off] = '\\';
	off++;
	ret[off] = '&';
    }
    else if(string[i] == '$' )
    {
	ret[off] = '\\';
	off++;
	ret[off] = '$';
    }
    else
    {
	ret[off] = string[i];
    }

    off++;
  }
  
  return ret;
}



void sctk_profile_render_tex_register( struct sctk_profile_renderer *rd )
{
	rd->setup = sctk_profile_render_tex_setup;
	rd->teardown = sctk_profile_render_tex_teardown;

	rd->setup_profile = sctk_profile_render_tex_setup_profile;
	rd->render_profile = sctk_profile_render_tex_render_profile;
	rd->teardown_profile = sctk_profile_render_tex_teardown_profile;

	rd->setup_meta = sctk_profile_render_tex_setup_meta;
	rd->render_meta = sctk_profile_render_tex_render_meta;
	rd->teardown_meta = sctk_profile_render_tex_setup_meta;


}


void sctk_profile_render_tex_setup( struct sctk_profile_renderer *rd )
{
	char buff[300];
	char output_file[500];
	
	sprintf( output_file, "mpc_profile_%s.tex", sctk_profile_renderer_date( buff ) );
	

	rd->output_file = fopen( output_file, "w" );
	
	if( !rd->output_file )
	{
		sctk_error( "Failed to open profile file : %s ", output_file );
		perror( " fopen " );
		abort();
	}


	fprintf(rd->output_file,
		"\\documentclass[french,landscape,a4paper]{report}\n"
		"\\usepackage[T1]{fontenc}\n"
		"\\usepackage{graphicx}\n"
		"\\usepackage{lscape}\n"
		"\\usepackage{babel}\n"
		"\\usepackage{subfigure}\n"
		"\\usepackage[table]{xcolor}\n"
		"\\definecolor{A}{RGB}{58,77,133}\n"
		"\\definecolor{B}{RGB}{130,162,255}\n"
		"\\definecolor{C}{RGB}{184,189,203}\n"
		"\\usepackage{hyperref}\n"
		"\\usepackage[latin9]{inputenc}\n"
		"\\usepackage{geometry}\n"
		"\\geometry{lmargin=2cm,rmargin=2cm}\n"
		"\\title{MPC Profile %s}\n"
		"\\begin{document}\n"
		"\\maketitle\n", sctk_profile_renderer_date_clean( buff ));
}


void sctk_profile_render_tex_teardown( struct sctk_profile_renderer *rd )
{
	fprintf(rd->output_file, "\\end{document}\n");
	fclose( rd->output_file );
	rd->output_file = NULL;

}


void sctk_profile_render_tex_setup_meta( struct sctk_profile_renderer *rd )
{



}


void sctk_profile_render_tex_render_meta( struct sctk_profile_renderer *rd, struct sctk_profile_meta *meta )
{


}


void sctk_profile_render_tex_teardown_meta( struct sctk_profile_renderer *rd )
{



}




void sctk_profile_render_tex_setup_profile( struct sctk_profile_renderer *rd )
{
	fprintf(rd->output_file,
			"\\begin{center}\n"
			"\\begin{tabular}{lccccccc}\n"
			"Name & Hits & Total Time & Average Time & Minimum Time & Maximum Time & Section & Global \\\\\n");
}


void sctk_profile_render_tex_render_profile( struct sctk_profiler_array *array, int id, int parent_id, int depth, struct sctk_profile_renderer *rd )
{
	char buffA[500], buffB[500], buffC[500], buffD[500];
	
	const char *prefix[3] = { "\\textbf", " ", "\\textit" };
	const char *colors[3] = { "A", "B", "C" };
	
	
	int prefix_id = (depth<3)?depth:3;

	char *to_unit_total = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_time(array, id) , buffA );
	char *to_unit_avg = sctk_profile_renderer_convert_to_time( rd->ptree.entry_average_time[id] , buffB );
	char *to_unit_min = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_min(array, id) , buffC );
	char *to_unit_max = sctk_profile_renderer_convert_to_time( sctk_profiler_array_get_max(array, id) , buffD );

	char *desc = sctk_profile_render_tex_sanitize_string( sctk_profiler_array_get_desc( id ) );

	if( sctk_profiler_array_get_hits( array, id ) )
	{
		fprintf( rd->output_file,"\\rowcolor{%s} %s{%s} & %s{%llu} & %s{%s} & %s{%s} & %s{%s} & %s{%s} & %s{ %g } & %s{ %g } \\\\\n",colors[ prefix_id], prefix[ prefix_id ], desc,
																						   prefix[ prefix_id ], (unsigned long long int )sctk_profiler_array_get_hits( array, id ),
																						   prefix[ prefix_id ], to_unit_total,
																						   prefix[ prefix_id ], to_unit_avg,
																						   prefix[ prefix_id ], to_unit_min,
																						   prefix[ prefix_id ], to_unit_max,
																						   prefix[ prefix_id ], rd->ptree.entry_relative_percentage_time[id] * 100,
																						   prefix[ prefix_id ], rd->ptree.entry_total_percentage_time[id] * 100 );
	}

	free(desc);
}



void sctk_profile_render_tex_teardown_profile( struct sctk_profile_renderer *rd )
{
	fprintf(rd->output_file, "\\end{tabular}\n"
							 "\\end{center}\n");
}


