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

#include "sctk_profiler_array.h"

typedef enum
{
	SCTK_PROFILE_RENDER_FILE,
	SCTK_PROFILE_RENDER_FILE_RAW,
	SCTK_PROFILE_RENDER_FILE_NOINDENT,
	SCTK_PROFILE_RENDER_STDOUT,
	SCTK_PROFILE_RENDER_STDOUT_RAW,
	SCTK_PROFILE_RENDER_STDOUT_NOINDENT,
	SCTK_PROFILE_RENDER_TEX,
	SCTK_PROFILE_RENDER_XML,
	SCTK_PROFILE_RENDER_HTML,
	SCTK_PROFILE_RENDER_COUNT
}sctk_profile_render_type;



static const char * const sctk_profile_render_keys[SCTK_PROFILE_RENDER_COUNT] =

{
	"file",
	"file-raw",
	"file-noindent",
	"stdout",
	"stdout-raw",
	"stdout-noindent",
	"latex",
	"xml",
	"html"
};




struct sctk_profile_renderer
{
	struct sctk_performance_tree ptree;
	struct sctk_profiler_array *array;
	char render_list[500];
	sctk_profile_render_walk_mode walk_mode;

	void (*setup)( struct sctk_profile_renderer *rd );
	void (*teardown)( struct sctk_profile_renderer *rd );

	void (*setup_meta)( struct sctk_profile_renderer *rd );
	void (*teardown_meta)( struct sctk_profile_renderer *rd );
	void (*render_meta)( struct sctk_profile_renderer *rd, struct sctk_profile_meta *meta );

	void (*setup_profile)( struct sctk_profile_renderer *rd );
	void (*teardown_profile)( struct sctk_profile_renderer *rd );
	void (*render_profile)( struct sctk_profiler_array *array, int id, int parent_id, int depth, int going_up, struct sctk_profile_renderer *rd );

	FILE *output_file;

};

static inline void sctk_profile_renderer_set_walk( struct sctk_profile_renderer *rd, sctk_profile_render_walk_mode mode )
{
	if( mode < SCTK_PROFILE_RENDER_WALK_COUNT )
		rd->walk_mode = mode;
}



void sctk_profile_renderer_init( struct sctk_profile_renderer *rd, struct sctk_profiler_array *array, char *render_string);

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

struct mpc_common_profiler_config
{
	int level_colors_size;
	int append_date;
	int color_stdout;
	char * file_prefix;
	char* level_colors[];
};

const struct mpc_common_profiler_config * sctk_profile_get_config();

void sctk_profile_render_filename( char *output_file, char *ext );

char * sctk_profile_render_sanitize_string( char *string );

#endif /* SCTK_PROFILE_RENDER */
