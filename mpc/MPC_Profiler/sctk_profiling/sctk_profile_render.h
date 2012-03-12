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

#ifndef SCTK_PROFILE_RENDER
#define SCTK_PROFILE_RENDER

#include <stdio.h>
#include <time.h>

#include "sctk_performance_tree.h"


typedef enum
{
	SCTK_PROFILE_RENDER_TEXT,
	SCTK_PROFILE_RENDER_TEXT_RAW,
	SCTK_PROFILE_RENDER_TEXT_NOINDENT,
	SCTK_PROFILE_RENDER_TEXT_STDOUT,
	SCTK_PROFILE_RENDER_TEXT_STDOUT_RAW,
	SCTK_PROFILE_RENDER_TEXT_STDOUT_NOINDENT,
	SCTK_PROFILE_RENDER_TEX,
	SCTK_PROFILE_RENDER_HTML,
	SCTK_PROFILE_RENDER_COUNT
}sctk_profile_render_type;

static const char * const sctk_profile_render_keys[] =

{
	"text",
	"text_raw",
	"text_noindent",
	"text_stdout",
	"text_stdout_raw",
	"text_stdout_noindent",
	"latex",
	"html"
};




struct sctk_profile_renderer
{
	struct sctk_performance_tree ptree;
	struct sctk_profiler_array *array;
	char render_list[500];


	void (*setup)( struct sctk_profile_renderer *rd );
	void (*teardown)( struct sctk_profile_renderer *rd );
	void (*render_entry)( struct sctk_profiler_array *array, int id, int parent_id, int depth, struct sctk_profile_renderer *rd );
	
	FILE *output_file;

};


void sctk_profile_renderer_init( struct sctk_profile_renderer *rd, struct sctk_profiler_array *array, char *render_string );

void sctk_profile_renderer_render( struct sctk_profile_renderer *rd );

void sctk_profile_renderer_release( struct sctk_profile_renderer *rd );

char *sctk_profile_renderer_convert_to_time( uint64_t duration , char *to_unit );

int sctk_profile_renderer_check_render_list( char *render_list );



static char * sctk_profile_renderer_date( char *buffer )
{
	time_t t = time( NULL );
	struct tm *time_entry = localtime( &t );
	
	if( strftime( buffer, 300, "%F_%H-%M-%S", time_entry ) <= 0 )
	{
		buffer[0] = '\0';
	}

	
	return buffer;
}

static char * sctk_profile_renderer_date_clean( char *buffer )
{
	time_t t = time( NULL );
	struct tm *time_entry = localtime( &t );
	
	if( strftime( buffer, 300, "%F %H:%M:%S", time_entry ) <= 0 )
	{
		buffer[0] = '\0';
	}

	
	return buffer;
}



static void sctk_profile_renderer_write_ntabs( FILE *fd, int n )
{
	int i = 0;
	
	for( i = 0 ; i < n ; i++ )
	{
		fprintf( fd, "    ");
	}

}

static void sctk_profile_renderer_print_ntabs( int n )
{
	int i = 0;
	
	for( i = 0 ; i < n ; i++ )
	{
		printf( "    ");
	}

}


#endif /* SCTK_PROFILE_RENDER */
