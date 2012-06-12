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
#include "sctk_profile_meta.h"
#include "sctk_runtime_config.h"


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

	void (*setup_meta)( struct sctk_profile_renderer *rd );
	void (*teardown_meta)( struct sctk_profile_renderer *rd );
	void (*render_meta)( struct sctk_profile_renderer *rd, struct sctk_profile_meta *meta );

	void (*setup_profile)( struct sctk_profile_renderer *rd );
	void (*teardown_profile)( struct sctk_profile_renderer *rd );
	void (*render_profile)( struct sctk_profiler_array *array, int id, int parent_id, int depth, struct sctk_profile_renderer *rd );
	
	FILE *output_file;

};


void sctk_profile_renderer_init( struct sctk_profile_renderer *rd, struct sctk_profiler_array *array, char *render_string );

void sctk_profile_renderer_render( struct sctk_profile_renderer *rd );

void sctk_profile_renderer_release( struct sctk_profile_renderer *rd );

char *sctk_profile_renderer_convert_to_size( uint64_t size , char *to_size );

char *sctk_profile_renderer_convert_to_time( uint64_t duration , char *to_unit );

int sctk_profile_renderer_check_render_list( char *render_list );

/* Utility functions */

char * sctk_profile_renderer_date( char *buffer );

char * sctk_profile_renderer_date_clean( char *buffer );

void sctk_profile_renderer_write_ntabs( FILE *fd, int n );

void sctk_profile_renderer_print_ntabs( int n );

struct MPC_prof_color
{
	int r;
	int g;
	int b;
};

struct MPC_prof_color sctk_profile_renderer_to_rgb( char *hex_col );

const struct sctk_runtime_config_struct_profiler * sctk_profile_get_config();
void sctk_profile_render_filename( char *output_file, char *ext );

#endif /* SCTK_PROFILE_RENDER */
