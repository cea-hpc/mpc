/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <execinfo.h>
#include <signal.h>
#include <mpc_config.h>

#include "mpc_common_rank.h"
#include "sctk_debug.h"
#include "mpc_common_spinlock.h"
#include "mpc_common_helper.h"


#ifdef MPC_Thread_db
	#include <sctk_debugger.h>
#endif

#define SMALL_BUFFER_SIZE ( 4 * 1024 )
#define DEBUG_INFO_SIZE ( 64 )

int sctk_is_in_fortran = 0;


/**********************************************************************/
/*Threads support                                                     */
/**********************************************************************/

static inline char *__debug_print_info( char *buffer )
{
#ifdef MPC_ENABLE_SHELL_COLORS
		snprintf( buffer,
		          DEBUG_INFO_SIZE,
		          SCTK_COLOR_GREEN( [ )
		                            SCTK_COLOR_RED(R%4d)
		                            SCTK_COLOR_BLUE( P%4dN%4d)
		                            SCTK_COLOR_GREEN( ] ),
		          mpc_common_get_task_rank(), mpc_common_get_process_rank(), mpc_common_get_node_rank() );
#else
		snprintf( buffer,
		          DEBUG_INFO_SIZE,
		          "[R%4dP%4dN%4d]",
		          mpc_common_get_task_rank(), mpc_common_get_process_rank(), mpc_common_get_node_rank() );
#endif

	return buffer;
}

/**********************************************************************/
/*Abort                                                               */
/**********************************************************************/
#ifdef MPC_Launch
	void mpc_launch_pmi_abort();
#endif
void sctk_abort( void )
{
	mpc_common_debuger_print_backtrace( "sctk_abort" );
	abort();
}


/**********************************************************************/
/*Messages                                                            */
/**********************************************************************/

int MPC_check_compatibility_lib( int major, int minor, char *pre )
{
	static char errro_msg[4096];

	if ( ( major != MPC_VERSION_MAJOR ) || ( minor != MPC_VERSION_MINOR ) || ( strcmp( pre, MPC_VERSION_PRE ) != 0 ) )
	{
		sprintf( errro_msg,
		         "MPC version used for this file (%d.%d%s) differs from the library used for the link (%d.%d%s)\n",
		         major, minor, pre, MPC_VERSION_MAJOR, MPC_VERSION_MINOR,
		         MPC_VERSION_PRE );
		sctk_warning( errro_msg );
		return 1;
	}

	return 0;
}

void MPC_printf( const char *fmt, ... )
{
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];
	va_start( ap, fmt );
	mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
	                                "%s %s",
	                                __debug_print_info( debug_info ),
	                                fmt );
	mpc_common_io_noalloc_vfprintf( stderr, buff, ap );
	fflush( stderr );
	va_end( ap );
}

/* Sometimes it's interesting if only rank 0 show messages... */
void sctk_debug_root( const char *fmt, ... )
{
	va_list ap;
	char buff[SMALL_BUFFER_SIZE];

	if ( mpc_common_get_process_rank() != 0 )
	{
		return;
	}

	va_start( ap, fmt );

#ifdef MPC_ENABLE_SHELL_COLORS
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE, SCTK_COLOR_RED_BOLD( "%s" ) "\n", fmt );
#else
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE, "%s\n", fmt );
#endif

	mpc_common_io_noalloc_vfprintf( stderr, buff, ap );
	fflush( stderr );
	va_end( ap );
}

void mpc_common_debug_log( const char *fmt, ... )
{
	if ( mpc_common_get_flags()->verbosity < 1 )
	{
		return;
	}

        va_list ap;
        char buff[SMALL_BUFFER_SIZE];
        va_start( ap, fmt );

#ifdef MPC_ENABLE_SHELL_COLORS
        mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE, ""SCTK_COLOR_GREEN_BOLD( %s )"\n", fmt );

#else
        mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
                                        "%s\n",
                                        fmt );
#endif

	mpc_common_io_noalloc_vfprintf( stderr, buff, ap );
	va_end( ap );
}

void mpc_common_debug_info( const char *fmt, ... )
{
	if ( mpc_common_get_flags()->verbosity < 2 )
	{
		return;
	}

	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];
	va_start( ap, fmt );

#ifdef MPC_ENABLE_SHELL_COLORS
		char info_message[SMALL_BUFFER_SIZE];
		mpc_common_io_noalloc_snprintf( info_message, SMALL_BUFFER_SIZE, SCTK_COLOR_GRAY_BOLD( "%s" ), fmt );
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s INFO %s\n",
		                                __debug_print_info( debug_info ),
		                                info_message );
#else
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s INFO %s\n",
		                                __debug_print_info( debug_info ),
		                                fmt );
#endif

	mpc_common_io_noalloc_vfprintf( stderr, buff, ap );
	va_end( ap );
}

#ifdef MPC_ENABLE_DEBUG_MESSAGES

void mpc_common_debug( const char *fmt, ... )
{
#if defined( MPC_Message_Passing ) || defined( MPC_Threads )

	if ( mpc_common_get_flags()->verbosity < 3 )
	{
		return;
	}

#endif
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];
	va_start( ap, fmt );

#ifdef MPC_ENABLE_SHELL_COLORS
		char debug_message[SMALL_BUFFER_SIZE];
		mpc_common_io_noalloc_snprintf( debug_message, SMALL_BUFFER_SIZE, SCTK_COLOR_CYAN_BOLD( "%s" ), fmt );
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s DEBUG %s\n",
		                                __debug_print_info( debug_info ),
		                                debug_message );
#else
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s DEBUG %s\n",
		                                __debug_print_info( debug_info ),
		                                fmt );
#endif

	mpc_common_io_noalloc_vfprintf( stderr, buff, ap );
	fflush( stderr );
	va_end( ap );
}
#endif

void sctk_error( const char *fmt, ... )
{
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];
	va_start( ap, fmt );

#ifdef MPC_ENABLE_SHELL_COLORS
		char error_message[SMALL_BUFFER_SIZE];
		mpc_common_io_noalloc_snprintf( error_message, SMALL_BUFFER_SIZE, SCTK_COLOR_RED_BOLD( "%s" ), fmt );
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s ERROR %s\n",
		                                __debug_print_info( debug_info ),
		                                error_message );
#else
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s ERROR %s\n",
		                                __debug_print_info( debug_info ),
		                                fmt );
#endif

	mpc_common_io_noalloc_vfprintf( stderr, buff, ap );
	va_end( ap );
}

void sctk_formated_assert_print( FILE *stream, const int line, const char *file,
                                 const char *func, const char *fmt, ... )
{
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];

	if ( func == NULL )
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s Assertion %s fail at line %d file %s\n",
		                                __debug_print_info( debug_info ),
		                                fmt, line,
		                                file );
	else
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s Assertion %s fail at line %d file %s func %s\n",
		                                __debug_print_info( debug_info ),
		                                fmt, line, file, func );

	va_start( ap, fmt );
	mpc_common_io_noalloc_vfprintf( stream, buff, ap );
	va_end( ap );
	sctk_abort();
}


void mpc_common_debug_log_file( FILE *file, const char *fmt, ... )
{
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];
	va_start( ap, fmt );
	mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
	                                "%s %s\n",
	                                __debug_print_info( debug_info ),
	                                fmt );
	vfprintf( file, buff, ap );
	va_end( ap );
	fflush( file );
}

void sctk_warning( const char *fmt, ... )
{
	/* #if defined(MPC_Message_Passing) || defined(MPC_Threads) */
	/*   if( mpc_common_get_flags()->verbosity < 1 ) */
	/*     return; */
	/* #endif */
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];
	va_start( ap, fmt );

#ifdef MPC_ENABLE_SHELL_COLORS
		char warning_message[SMALL_BUFFER_SIZE];
		mpc_common_io_noalloc_snprintf( warning_message, SMALL_BUFFER_SIZE, SCTK_COLOR_YELLOW_BOLD( "%s" ), fmt );
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s WARNING %s\n",
		                                __debug_print_info( debug_info ),
		                                warning_message );
#else
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s WARNING %s\n",
		                                __debug_print_info( debug_info ),
		                                fmt );
#endif

	mpc_common_io_noalloc_vfprintf( stderr, buff, ap );
	va_end( ap );
}

void sctk_formated_dbg_print_abort( FILE *stream, const int line,
                                    const char *file, const char *func,
                                    const char *fmt, ... )
{
	va_list ap;
	char buff[SMALL_BUFFER_SIZE];

	if ( func == NULL )
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE, "Debug at line %d file %s: %s\n",
		                                line, file, fmt );
	else
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "Debug at line %d file %s func %s: %s\n", line, file, func,
		                                fmt );

	va_start( ap, fmt );
	mpc_common_io_noalloc_vfprintf( stream, buff, ap );
	va_end( ap );
	sctk_abort();
}

/**********************************************************************/
/*Sizes                                                               */
/**********************************************************************/
void sctk_size_checking( size_t a, size_t b, char *ca, char *cb, char *file,
                         int line )
{
	if ( !( a <= b ) )
	{
		mpc_common_io_noalloc_fprintf( stderr,
		                               "Internal error !(%s <= %s) at line %d in %s\n",
		                               ca, cb, line, file );
		abort();
	}
}
void sctk_size_checking_eq( size_t a, size_t b, char *ca, char *cb, char *file,
                            int line )
{
	if ( a != b )
	{
		mpc_common_io_noalloc_fprintf( stderr,
		                               "Internal error %s != %s at line %d in %s\n",
		                               ca, cb, line, file );
		abort();
	}
}
