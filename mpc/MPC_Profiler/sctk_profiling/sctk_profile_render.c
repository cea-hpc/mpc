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
#include "sctk_profile_render.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "sctk_profiler_array.h"
#include "sctk_debug.h"
#include "sctk_asm.h"

/* Profile renderer list */
#include "sctk_profile_render_text.h"
#include "sctk_profile_render_tex.h"

/* Profile renderer Interface */
void sctk_profile_renderer_register_output_iface( struct sctk_profile_renderer *rd, sctk_profile_render_type render_type )
{
	switch( render_type )
	{
		
		case SCTK_PROFILE_RENDER_TEXT :
			sctk_profile_render_text_register( rd, 0 );
		break;
		case SCTK_PROFILE_RENDER_TEXT_STDOUT :
			sctk_profile_render_text_register( rd, 1 );
		break;
		case SCTK_PROFILE_RENDER_TEX :
			sctk_profile_render_tex_register( rd );
		break;
		case SCTK_PROFILE_RENDER_HTML :
			sctk_profile_render_html_register( rd );
		break;
		default :
			return;
	}
	
	
}

void sctk_profile_renderer_remove_output_iface( struct sctk_profile_renderer *rd )
{
	rd->setup = NULL;
	rd->teardown = NULL;
	rd->render_entry = NULL;

	if( rd->output_file )
	{
		sctk_error( "An output interface left a non-null file ptr did you close it ?" );
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
	
	int i = 0;
	
	for( i = 0 ; i < len ; i++ )
	{
		if( sctk_profile_renderer_is_alpha( render_list[i] ) 
		    && sctk_profile_renderer_is_numeric( render_list[i] ) 
		    && sctk_profile_renderer_is_comma( render_list[i] )  
		    && sctk_profile_renderer_is_authorized_symbol( render_list[i] ) 
		  )
		 {
			sctk_warning("Render list contains unauthorized character : '%c' ", render_list[i]);
			return 1;
		 }
	}

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




static double sctk_profile_renderer_avg_ticks_per_sec;


double sctk_profile_renderer_init_timer()
{
        double ret = 0;
        
        uint64_t begin = 0, end = 0;
        double t_begin = 0, t_end = 0;

        struct timeval b_tv;
        struct timeval e_tv;

        //printf( "Calibrating timer...");
        gettimeofday( &b_tv , NULL );
        begin = sctk_get_time_stamp();
        sleep( 1 );
        end = sctk_get_time_stamp();
        gettimeofday( &e_tv , NULL );

        t_begin = b_tv.tv_sec + b_tv.tv_usec/1000000;
        t_end = e_tv.tv_sec + e_tv.tv_usec/1000000;

        ret = (end-begin)/(t_end-t_begin);

        return ret;
}



void sctk_profile_renderer_init( struct sctk_profile_renderer *rd, struct sctk_profiler_array *array, char *render_list )
{
	
	sctk_performance_tree_init( &rd->ptree, array);
	rd->array = array;
	
	if( sctk_profile_renderer_check_render_list( render_list ) )
	{
		sctk_error( "Provided render list syntax is not correct: %s", render_list );
		abort();
	}

	strncpy( rd->render_list, render_list, 500 );

	rd->setup = NULL;
	rd->teardown = NULL;
	rd->render_entry = NULL;
	rd->output_file = NULL;
	
	sctk_info( "Calibrating timer...");
	
	sctk_profile_renderer_avg_ticks_per_sec = sctk_profile_renderer_init_timer();

}


void sctk_profile_renderer_release( struct sctk_profile_renderer *rd )
{
	sctk_performance_tree_release( &rd->ptree );
	rd->array = NULL;

	rd->setup = NULL;
	rd->teardown = NULL;
	rd->render_entry = NULL;
	rd->output_file = NULL;
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



void sctk_profile_renderer_render_entry( struct sctk_profiler_array *array, int id, int parent_id, int depth, void *arg )
{
	struct sctk_profile_renderer *rd = (struct sctk_profile_renderer *)arg;
	
	if( rd->render_entry )
		(rd->render_entry)( array, id, parent_id, depth, rd );
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
			sctk_warning( "No such renderer type : %s", render_string );
			continue;
		}

		sctk_info("Rendering %s profile", render_string);


		sctk_profile_renderer_register_output_iface( rd, rd_type);
		
			if( rd->setup )
				(rd->setup )( rd );
				
			if( rd->render_entry )
				sctk_profiler_array_walk( rd->array, sctk_profile_renderer_render_entry, (void *)rd, 0 );

			if( rd->teardown )
				(rd->teardown )( rd );

		sctk_profile_renderer_remove_output_iface( rd );
		
		
	}
}



char *sctk_profile_renderer_convert_to_time( uint64_t duration , char *to_unit )
{
	  
	  if( duration == 0 )
	  {
		sprintf(to_unit, "0");
		return to_unit;
	  }


	  double time_sec = duration / (sctk_profile_renderer_avg_ticks_per_sec);
	 
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





