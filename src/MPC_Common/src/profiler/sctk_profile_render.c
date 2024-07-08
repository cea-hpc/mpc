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

#include "sctk_profile_render.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "sctk_profiler_array.h"
#include "mpc_common_debug.h"
#include "mpc_common_asm.h"

/* Profile renderer list */
#include "sctk_internal_profiler.h"
#include "sctk_profile_render_text.h"
#include "sctk_profile_render_tex.h"
#include "sctk_profile_render_html.h"
#include "sctk_profile_render_xml.h"

void sctk_profiler_renderer_clear_handlers( struct sctk_profile_renderer *rd )
{
	rd->setup = NULL;
	rd->teardown = NULL;

	rd->setup_meta = NULL;
	rd->setup_profile = NULL;

	rd->teardown_meta = NULL;
	rd->teardown_profile = NULL;


	rd->render_profile = NULL;
	rd->output_file = NULL;
	rd->render_meta = NULL;
}




/* Profile renderer Interface */
void sctk_profile_renderer_register_output_iface( struct sctk_profile_renderer *rd, sctk_profile_render_type render_type )
{
	sctk_profiler_renderer_clear_handlers( rd );

	switch( render_type )
	{

		case SCTK_PROFILE_RENDER_FILE :
			sctk_profile_render_text_register( rd, 0, 0, 0 );
		break;
		case SCTK_PROFILE_RENDER_FILE_RAW :
			sctk_profile_render_text_register( rd, 0, 1, 1 );
		break;
		case SCTK_PROFILE_RENDER_FILE_NOINDENT :
			sctk_profile_render_text_register( rd, 0, 0, 1 );
		break;
		case SCTK_PROFILE_RENDER_STDOUT :
			sctk_profile_render_text_register( rd, 1, 0, 0 );
		break;
		case SCTK_PROFILE_RENDER_STDOUT_RAW :
			sctk_profile_render_text_register( rd, 1, 1, 1 );
		break;
		case SCTK_PROFILE_RENDER_STDOUT_NOINDENT :
			sctk_profile_render_text_register( rd, 1, 0, 1 );
		break;
		case SCTK_PROFILE_RENDER_TEX :
			sctk_profile_render_tex_register( rd );
		break;
		case SCTK_PROFILE_RENDER_XML :
                  //	printf("Reg XML ! \n");
                  sctk_profile_render_xml_register(rd);
                  break;
                case SCTK_PROFILE_RENDER_HTML:
                  sctk_profile_render_html_register(rd);
                  break;
                default:
                  return;
                }
}

void sctk_profile_renderer_remove_output_iface( struct sctk_profile_renderer *rd )
{
	rd->setup = NULL;
	rd->teardown = NULL;
	rd->render_profile = NULL;

	if( rd->output_file )
	{
		mpc_common_debug_error( "An output interface left a non-null file ptr did you close it ?" );
		abort();
	}
}





static int sctk_profile_renderer_is_alpha( char c )
{
	if( c <= 'z' && 'a' <= c )
	{
		return 0;
	}

	if( c <= 'Z' && 'A' <= c )
		return 0;

	return 1;
}

static int sctk_profile_renderer_is_numeric( char c )
{
	if( c <= '9' && '0' <= c )
		return 0;

	return 1;
}

static int sctk_profile_renderer_is_comma( char c )
{
	if( c == ',' )
		return 0;

	return 1;
}

static int sctk_profile_renderer_is_authorized_symbol( char c )
{
	if( c == '-' ||  c == '_' )
		return 0;

	return 1;
}




int sctk_profile_renderer_check_render_list( char *render_list )
{
	int len = strlen( render_list );

	render_list = strdup(render_list);

	int i = 0;

	for( i = 0 ; i < len ; i++ )
	{
		if( sctk_profile_renderer_is_alpha( render_list[i] )
		    && sctk_profile_renderer_is_numeric( render_list[i] )
		    && sctk_profile_renderer_is_comma( render_list[i] )
		    && sctk_profile_renderer_is_authorized_symbol( render_list[i] )
		  )
		 {
			mpc_common_debug_warning("Render list contains unauthorized character : '%c' ", render_list[i]);
			return 1;
		 }
	}

	free(render_list);

	return 0;
}


int sctk_profile_renderer_render_sting_to_id( char *render_string )
{
	int i = 0;

	for( i = 0 ; i < SCTK_PROFILE_RENDER_COUNT; i++ )
	{
		if( !strcasecmp( render_string, sctk_profile_render_keys[i] ) )
			return i;
	}

	return -1;
}


void sctk_profile_renderer_init( struct sctk_profile_renderer *rd, struct sctk_profiler_array *array, char *render_list )
{

	sctk_performance_tree_init( &rd->ptree, array);
	rd->array = array;
	rd->walk_mode = SCTK_PROFILE_RENDER_WALK_DFS;

	if( sctk_profile_renderer_check_render_list( render_list ) )
	{
		mpc_common_debug_error( "Provided render list syntax is not correct: %s", render_list );
		abort();
	}

	strncpy( rd->render_list, render_list, 500 );

	sctk_profiler_renderer_clear_handlers( rd );

}


void sctk_profile_renderer_release( struct sctk_profile_renderer *rd )
{
	sctk_performance_tree_release( &rd->ptree );

	sctk_profiler_renderer_clear_handlers( rd );
}



char * sctk_profile_renderer_get_next_render_string( char **render_list )
{



	if( ! (*render_list) )
		return NULL;

	int len = strlen( *render_list );

	if( len == 0 )
		return NULL;


	char *comma = strstr( *render_list, "," );
	char *ret = NULL;

	/* Last entry */
	if( !comma )
	{
		ret = *render_list;
		*render_list = NULL;
		return ret;
	}
	else
	{
		*comma = '\0';
		ret = *render_list;
		*render_list = comma + 1;
	}

	return ret;

}



void sctk_profile_renderer_render_entry( struct sctk_profiler_array *array, int id, int parent_id, int depth, void *arg, int going_up )
{
	struct sctk_profile_renderer *rd = (struct sctk_profile_renderer *)arg;

	if( rd->render_profile )
		(rd->render_profile)( array, id, parent_id, depth, going_up, rd);
}



void sctk_profile_renderer_render( struct sctk_profile_renderer *rd )
{
	char local_list[500];

	if( !strcmp( rd->render_list, "none" ) )
	{
		/* Nothing to render */
		return;
	}

	strncpy( local_list, rd->render_list, 500 );

	char *render_string = NULL;
	char *render_list = local_list;

	while( ( render_string = sctk_profile_renderer_get_next_render_string( &render_list ) ) )
	{
		int rd_type = sctk_profile_renderer_render_sting_to_id( render_string );

		if( rd_type < 0 )
		{
			mpc_common_debug_warning( "No such renderer type : %s", render_string );
			continue;
		}

		mpc_common_debug("Rendering %s profile", render_string);


		sctk_profile_renderer_register_output_iface( rd, rd_type);

			/* SETUP */
			if( rd->setup )
				(rd->setup )( rd );

			/* META */
			if( rd->setup_meta )
				(rd->setup_meta )( rd );

           if( rd->render_meta )
        	   (rd->render_meta)(rd, sctk_internal_profiler_get_meta());

			if( rd->teardown_meta )
				(rd->teardown_meta )( rd );

			/* PROFILE */
			if( rd->setup_profile )
				(rd->setup_profile )( rd );

			if( rd->render_profile )
				sctk_profiler_array_walk( rd->array, sctk_profile_renderer_render_entry, (void *)rd, rd->walk_mode );

			if( rd->teardown_profile )
				(rd->teardown_profile )( rd );

			/* TEARDOWN */
			if( rd->teardown )
				(rd->teardown )( rd );

		sctk_profile_renderer_remove_output_iface( rd );


	}
}

char *sctk_profile_renderer_convert_to_size( uint64_t size , char *to_size )
{

	if( size < 1024 ) {
		sprintf(to_size, "%ld B", size);
	} else if( size < 1024*1024 ) {
		sprintf(to_size, "%.2f KB", (float)size/1024);
	} else if( size < 1024*1024*1024 ) {
		sprintf(to_size, "%.2f MB", (float)size/(1024*1024));
	} else {
		sprintf(to_size, "%.2f GB", (float)size/(1073741824llu));
	}

	return to_size;
}

char *sctk_profile_renderer_convert_to_time( uint64_t duration , char *to_unit )
{

	  if( duration == 0 )
	  {
		sprintf(to_unit, "0");
		return to_unit;
	  }


	  double time_sec = duration / (sctk_internal_profiler_get_meta()->ticks_per_second);

	  uint64_t hour = floor(time_sec/3600);
	  uint64_t min = floor((double)(time_sec - hour*3600)/60);
	  uint64_t sec = time_sec - hour*3600 - min * 60;

     if( time_sec < 1e-6 )
	 {
	   sprintf(to_unit, "%.4g ns", time_sec * 1e9 );
	 }
	 else if( time_sec < 1e-3 )
	 {
	   sprintf(to_unit, "%.4g us", time_sec * 1e6 );
	 }
	 else if( time_sec < 1 )
	 {
	   sprintf(to_unit, "%.4g ms", time_sec * 1e3 );
	 }
	 else if( time_sec < 60 )
	 {
	   sprintf(to_unit, "%.4g s", time_sec );
	 }
	 else if( time_sec < 3600 )
	 {
	   sprintf(to_unit, "%ld min %ld s", min, sec );
	 }
	 else
	 {
	   sprintf(to_unit, "%ld hours %ld min %ld s", hour, min, sec );
	 }

	 return to_unit;
}


/* Utility functions */

char * sctk_profile_renderer_date( char *buffer )
{
	time_t t = time( NULL );
	struct tm *time_entry = localtime( &t );

	if( strftime( buffer, 300, "%F_%H-%M-%S", time_entry ) <= 0 )
	{
		buffer[0] = '\0';
	}


	return buffer;
}

char * sctk_profile_renderer_date_clean( char *buffer )
{
	time_t t = time( NULL );
	struct tm *time_entry = localtime( &t );

	if( strftime( buffer, 300, "%F %H:%M:%S", time_entry ) <= 0 )
	{
		buffer[0] = '\0';
	}


	return buffer;
}



void sctk_profile_renderer_write_ntabs( FILE *fd, int n )
{
	int i = 0;

	for( i = 0 ; i < n ; i++ )
	{
		fprintf( fd, "    |");
	}

}

void sctk_profile_renderer_print_ntabs( int n )
{
	int i = 0;

	for( i = 0 ; i < n ; i++ )
	{
		printf( "    ");
	}

}


struct MPC_prof_color sctk_profile_renderer_to_rgb( char *hex_col )
{

	struct MPC_prof_color ret = { 255, 255, 255 };

	int r, g, b;

	if( sscanf(hex_col,"#%2x%2x%2x",&r,&g,&b) == 3 )
	{
		ret.r = r;
		ret.g = g;
		ret.b = b;
	}

	return ret;
}

struct mpc_common_profiler_config __prof_config = {
	3,
	1,
	1,
	"mpc_profile",
	{"#FF0000", "#00FF00", "#0000FF"}
};

const struct mpc_common_profiler_config * sctk_profile_get_config()
{
	return (struct mpc_common_profiler_config *)&__prof_config;
}


void sctk_profile_render_filename( char *output_file, char *ext )
{
	char buff[500];
	if( sctk_profile_get_config()->append_date )
	{
		sprintf( output_file, "%s_%s.%s", sctk_profile_get_config()->file_prefix, sctk_profile_renderer_date( buff ), ext );
	}
	else
	{
		sprintf( output_file, "%s.%s", sctk_profile_get_config()->file_prefix, ext);
	}

	mpc_common_debug("MPC_Profiler : Rendering in %s", output_file);
}


char *sctk_profile_render_sanitize_string( char *string )
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

#endif /* MPC_Profiler */
