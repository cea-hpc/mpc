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
#include "sctk_profile_render_xml.h"

#include <stdlib.h>
#include <string.h>
#include "mpc_common_debug.h"



void sctk_profile_render_xml_register( struct sctk_profile_renderer *rd )
{
	rd->setup = sctk_profile_render_xml_setup;
	rd->teardown = sctk_profile_render_xml_teardown;

	rd->setup_profile = sctk_profile_render_xml_setup_profile;
	rd->render_profile = sctk_profile_render_xml_render_profile;
	rd->teardown_profile = sctk_profile_render_xml_teardown_profile;

	rd->setup_meta = sctk_profile_render_xml_setup_meta;
	rd->render_meta = sctk_profile_render_xml_render_meta;
	rd->teardown_meta = sctk_profile_render_xml_setup_meta;

	sctk_profile_renderer_set_walk( rd, SCTK_PROFILE_RENDER_WALK_BOTH );
}


void sctk_profile_render_xml_setup( struct sctk_profile_renderer *rd )
{
	char buff[300];
	char output_file[500];

	sctk_profile_render_filename( output_file, "xml" );
	rd->output_file = fopen( output_file, "w" );

	if( !rd->output_file )
	{
		mpc_common_debug_error( "Failed to open profile file : %s ", output_file );
		perror( " fopen " );
		abort();
	}


	fprintf(rd->output_file, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");

	fprintf(rd->output_file, "<MPC_Profiler>\n");

	fprintf(rd->output_file, "<colors>\n");
	int i = 0;

	for( i = 0 ; i < sctk_profile_get_config()->level_colors_size ; i++ )
	{
		struct MPC_prof_color col = sctk_profile_renderer_to_rgb( sctk_profile_get_config()->level_colors[i] );
		fprintf(rd->output_file, "\t<color_%d r=\"%d\" g=\"%d\" b=\"%d\"/>\n", i, col.r, col.g, col.b);
	}

	fprintf(rd->output_file, "</colors>\n");

}


void sctk_profile_render_xml_teardown( struct sctk_profile_renderer *rd )
{
	fprintf(rd->output_file, "</MPC_Profiler>\n");
	fclose( rd->output_file );
	rd->output_file = NULL;

}


void sctk_profile_render_xml_setup_meta( struct sctk_profile_renderer *rd )
{



}


void sctk_profile_render_xml_render_meta( struct sctk_profile_renderer *rd, struct sctk_profile_meta *meta )
{


}


void sctk_profile_render_xml_teardown_meta( struct sctk_profile_renderer *rd )
{



}




void sctk_profile_render_xml_setup_profile( struct sctk_profile_renderer *rd )
{
	fprintf(rd->output_file,"<profile>\n");
}


void sctk_profile_render_xml_print_n_time( FILE *f, char *t, int n)
{
	int i;
	for( i = 0 ; i < n ; i++)
		fprintf( f, "%s", t );
}




void sctk_profile_render_xml_render_profile( struct sctk_profiler_array *array, int id, int parent_id, int depth,  int going_up, struct sctk_profile_renderer *rd )
{

	if( going_up )
	{
		if( sctk_profiler_array_get_hits( array, id ) )
		{
			sctk_profile_render_xml_print_n_time( rd->output_file, "\t", depth);
			fprintf( rd->output_file, "</probe>\n");
		}
	}
	else
	{
		char buffA[100], buffB[100], buffC[100], buffD[100];

		int prefix_id = (depth<sctk_profile_get_config()->level_colors_size)?depth:sctk_profile_get_config()->level_colors_size - 1;

		int format_id = (prefix_id < 3)?prefix_id:2;

		char *to_unit_total = NULL;
		char *to_unit_avg = NULL;
		char *to_unit_min = NULL;
		char *to_unit_max = NULL;

                char *desc = sctk_profiler_array_get_desc(id);

                if (sctk_profiler_array_get_type(id) !=
                    SCTK_PROFILE_COUNTER_PROBE) {
                  if (sctk_profiler_array_get_type(id) ==
                      SCTK_PROFILE_COUNTER_SIZE_PROBE) {
                    to_unit_total = sctk_profile_renderer_convert_to_size(
                        sctk_profiler_array_get_value(array, id), buffA);
                    to_unit_avg = sctk_profile_renderer_convert_to_size(
                        rd->ptree.entry_average_time[id], buffB);
                    to_unit_min = sctk_profile_renderer_convert_to_size(
                        sctk_profiler_array_get_min(array, id), buffC);
                    to_unit_max = sctk_profile_renderer_convert_to_size(
                        sctk_profiler_array_get_max(array, id), buffD);
                  } else if (sctk_profiler_array_get_type(id) ==
                             SCTK_PROFILE_TIME_PROBE) {
                    to_unit_total = sctk_profile_renderer_convert_to_time(
                        sctk_profiler_array_get_value(array, id), buffA);
                    to_unit_avg = sctk_profile_renderer_convert_to_time(
                        rd->ptree.entry_average_time[id], buffB);
                    to_unit_min = sctk_profile_renderer_convert_to_time(
                        sctk_profiler_array_get_min(array, id), buffC);
                    to_unit_max = sctk_profile_renderer_convert_to_time(
                        sctk_profiler_array_get_max(array, id), buffD);
                  }
                } else {
                  sprintf(buffA, "%llu",
                          (unsigned long long int)sctk_profiler_array_get_value(
                              array, id));
                  to_unit_total = buffA;

                  sprintf(
                      buffB, "%llu",
                      (unsigned long long int)rd->ptree.entry_average_time[id]);
                  to_unit_avg = buffB;

                  sprintf(buffC, "%llu",
                          (unsigned long long int)sctk_profiler_array_get_min(
                              array, id));
                  to_unit_min = buffC;

                  sprintf(buffD, "%llu",
                          (unsigned long long int)sctk_profiler_array_get_max(
                              array, id));
                  to_unit_max = buffD;
                }

                if (sctk_profiler_array_get_hits(array, id)) {
                  sctk_profile_render_xml_print_n_time(rd->output_file, "\t",
                                                       depth);
                  fprintf(rd->output_file, "<probe name=\"%s\">\n", desc);

                  sctk_profile_render_xml_print_n_time(rd->output_file, "\t",
                                                       depth + 1);
                  fprintf(rd->output_file, "<hits>%llu</hits>\n",
                          (unsigned long long int)sctk_profiler_array_get_hits(
                              array, id));
                  sctk_profile_render_xml_print_n_time(rd->output_file, "\t",
                                                       depth + 1);
                  fprintf(rd->output_file, "<total>%s</total>\n",
                          to_unit_total);
                  sctk_profile_render_xml_print_n_time(rd->output_file, "\t",
                                                       depth + 1);
                  fprintf(rd->output_file, "<average>%s</average>\n",
                          to_unit_avg);
                  sctk_profile_render_xml_print_n_time(rd->output_file, "\t",
                                                       depth + 1);
                  fprintf(rd->output_file, "<min>%s</min>\n", to_unit_min);
                  sctk_profile_render_xml_print_n_time(rd->output_file, "\t",
                                                       depth + 1);
                  fprintf(rd->output_file, "<max>%s</max>\n", to_unit_max);
                  sctk_profile_render_xml_print_n_time(rd->output_file, "\t",
                                                       depth + 1);
                  fprintf(rd->output_file,
                          "<absolute_percentage>%g</absolute_percentage>\n",
                          rd->ptree.entry_total_percentage_time[id] * 100);
                  sctk_profile_render_xml_print_n_time(rd->output_file, "\t",
                                                       depth + 1);
                  fprintf(rd->output_file,
                          "<relative_percentage>%g</relative_percentage>\n",
                          rd->ptree.entry_relative_percentage_time[id] * 100);
                  sctk_profile_render_xml_print_n_time(rd->output_file, "\t",
                                                       depth + 1);
                  fprintf(
                      rd->output_file, "<color>color_%d</color>\n",
                      (depth < sctk_profile_get_config()->level_colors_size)
                          ? (depth)
                          : (sctk_profile_get_config()->level_colors_size - 1));
                }

                free(desc);
        }
}



void sctk_profile_render_xml_teardown_profile( struct sctk_profile_renderer *rd )
{
	fprintf(rd->output_file, "</profile>\n");
}

#endif /* MPC_Profiler */
