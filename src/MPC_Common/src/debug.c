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
#include <mpc_common_rank.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_helper.h>

#include <mpc_common_debug.h>
#include <mpc_common_flags.h>


#ifdef MPC_Thread_db
	#include <sctk_debugger.h>
#endif

#define SMALL_BUFFER_SIZE ( 4 * 1024 )
#define DEBUG_INFO_SIZE ( 64 )

int __is_stderr_tty;

FILE *ultra_verbose;

char registered_modules[REGISTERED_MODULES_LENGTH];
int registered_modules_index  = 0;

int mpc_common_debug_is_stderr_tty(){
	return __is_stderr_tty;
}
void mpc_common_debug_init(){
	if(getenv("IS_STDERR_TTY"))
		__is_stderr_tty = 1;
	else 
		__is_stderr_tty = 0;

	ultra_verbose = fopen("./mpc_log.txt", "w");
}

/**********************************************************************/
/*Threads support                                                     */
/**********************************************************************/

static inline char *__debug_print_info( char *buffer )
{
#ifdef MPC_ENABLE_SHELL_COLORS
		if(mpc_common_debug_is_stderr_tty() && mpc_common_get_flags()->colors){
			snprintf( buffer,
					DEBUG_INFO_SIZE,
					MPC_COLOR_GREEN( [ )
										MPC_COLOR_RED(R%4d)
										MPC_COLOR_BLUE( P%4dN%4d)
										MPC_COLOR_GREEN( ] ),
					mpc_common_get_task_rank(), mpc_common_get_process_rank(), mpc_common_get_node_rank() );
		}
		else{
			snprintf( buffer,
					DEBUG_INFO_SIZE,
					"[R%4dP%4dN%4d]",
					mpc_common_get_task_rank(), mpc_common_get_process_rank(), mpc_common_get_node_rank() );

		}
#else
		snprintf( buffer,
		          DEBUG_INFO_SIZE,
		          "[R%4dP%4dN%4d]",
		          mpc_common_get_task_rank(), mpc_common_get_process_rank(), mpc_common_get_node_rank() );
#endif

	return buffer;
}
/* NOLINTEND(clang-diagnostic-unused-function) */

/**********************************************************************/
/*Abort                                                               */
/**********************************************************************/
void mpc_launch_pmi_abort();

__attribute__((__noreturn__)) void mpc_common_debug_abort( void )
{
	mpc_common_debug_error("######## Program will now abort ########");
	mpc_launch_pmi_abort();
	abort();
}

/**********************************************************************/
/*Messages                                                            */
/**********************************************************************/


const char * mpc_common_debug_get_basename(const char * path)
{
	char * ret = strrchr(path, '/');
	return ret?(ret + 1):path;
}


int MPC_check_compatibility_lib( int major, int minor, int patch, char *pre )
{
	static char errro_msg[4096];

	if ( ( major != MPC_VERSION_MAJOR ) || ( minor != MPC_VERSION_MINOR ) || (patch != MPC_VERSION_PATCH) )
	{
		sprintf( errro_msg,
		         "MPC version used for this file (%d.%d.%d%s) differs from the library used for the link (%d.%d.%d%s)\n",
		         major, minor, patch, pre, MPC_VERSION_MAJOR, MPC_VERSION_MINOR, MPC_VERSION_PATCH, MPC_VERSION_PRE );
		mpc_common_debug_warning( errro_msg );
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

#ifdef MPC_ENABLE_DEBUG_MESSAGES

void mpc_common_debug( const char *fmt, ... )
{
#if defined( MPC_Lowcomm ) || defined( MPC_Threads )

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
		if(mpc_common_debug_is_stderr_tty() && mpc_common_get_flags()->colors){
			mpc_common_io_noalloc_snprintf( debug_message, SMALL_BUFFER_SIZE, MPC_COLOR_CYAN_BOLD( "%s" ), fmt );
			mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
											"%s DEBUG %s\n",
											__debug_print_info( debug_info ),
											debug_message );
		}
		else mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s DEBUG %s\n",
		                                __debug_print_info( debug_info ),
		                                fmt );
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

void mpc_common_debug_assert_print( FILE *stream, int line, const char *file,
                                 const char *func, const char *fmt, ... )
{
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];

	if ( func == NULL )
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s %s:%d : Assertion %s\n",
		                                __debug_print_info( debug_info ),
												  mpc_common_debug_get_basename(file),
												  line,
		                                fmt);
	else
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s [%s] %s:%d : Assertion %s\n",
		                                __debug_print_info( debug_info ),
												  func,
												  mpc_common_debug_get_basename(file),
												  line,
		                                fmt);

	va_start( ap, fmt );
	mpc_common_io_noalloc_vfprintf( stream, buff, ap );
	va_end( ap );
	mpc_common_debug_abort();
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

void mpc_common_debug_abort_log( FILE *stream, int line,
                                    const char *file, const char *func,
                                    const char *fmt, ... )
{
	va_list ap;
	char buff[SMALL_BUFFER_SIZE];

#ifdef MPC_ENABLE_SHELL_COLORS
	if(mpc_common_debug_is_stderr_tty() && mpc_common_get_flags()->colors)
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
					"%s:%d (%s):\n"
					"-------------------------------------\n"
					MPC_COLOR_RED_BOLD("%s")"\n"
					"-------------------------------------\n"
					"\n", file, line, func?func:"??",
					fmt );
	else mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
					"%s:%d (%s):\n"
					"-------------------------------------\n"
					"%s\n"
					"-------------------------------------\n"
					"\n", file, line, func?func:"??",
					fmt );
#else
	mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
					"%s:%d (%s):\n"
					"-------------------------------------\n"
					"%s\n"
					"-------------------------------------\n"
					"\n", file, line, func?func:"??",
					fmt );
#endif

	va_start( ap, fmt );
	mpc_common_io_noalloc_vfprintf( stream, buff, ap );
	va_end( ap );

	exit(1);
}

/**********************************************************************/
/*Sizes                                                               */
/**********************************************************************/
void mpc_common_debug_check_large_enough( size_t a, size_t b, char *ca, char *cb, char *file,
                         int line )
{
	if ( !( a <= b ) )
	{
		mpc_common_io_noalloc_fprintf( stderr,
		                               "Internal error !(%s <= %s) at line %d in %s\n",
		                               ca, cb, line, file );
		mpc_common_debug_abort();
	}
}
void mpc_common_debug_check_size_equal( size_t a, size_t b, char *ca, char *cb, char *file,
                            int line )
{
	if ( a != b )
	{
		mpc_common_io_noalloc_fprintf( stderr,
		                               "Internal error %s != %s at line %d in %s\n",
		                               ca, cb, line, file );
		mpc_common_debug_abort();
	}
}

int __mpcprintf(char *messagebuffer, char *modulename, char *filename, int line, char *color){
	int r;
	int task_rank = mpc_common_get_task_rank();
	#ifdef MPC_ENABLE_SHELL_COLORS
		if(mpc_common_debug_is_stderr_tty() 
			&& mpc_common_get_flags()->colors){
			fprintf(stderr, 
				MPC_COLOR_BLUE(%s)
				MPC_COLOR_GREEN( [ )
				MPC_COLOR_RED(R%4d)
				MPC_COLOR_BLUE( P%4dN%4d)
				MPC_COLOR_GREEN( ] ) " %s%s%s\n",
				modulename,
				task_rank, 
				mpc_common_get_process_rank(), 
				mpc_common_get_node_rank(),
				color , messagebuffer, MPC_COLOR_DEFAULT_CHAR);
		}
		else{
			fprintf(stderr,"%s\n", messagebuffer);
		}
	#else
		fprintf(stderr, "%s", messagebuffer);
	#endif
	r = fprintf (ultra_verbose,
		"[R%4dP%4dN%4d] <%s:%d> %s\n",
		task_rank, 
		mpc_common_get_process_rank(), 
		mpc_common_get_node_rank(),
		filename, line, messagebuffer);
	return r;
}


int _common_debug(char *filename, int line, const char *funcname, char *color, int print_type, char *modulename, char *string, ...){
	va_list ap;
	int r;
	va_start (ap, string); 

	char messagebuffer[SMALL_BUFFER_SIZE];

	r = vsnprintf(messagebuffer,SMALL_BUFFER_SIZE, string, ap);
	if(print_type == PRINT_DEBUG){
		if(authorized_module(modulename) || authorized_function(funcname) || authorized_file(filename))
			__mpcprintf(messagebuffer, modulename, filename, line, color);
	}
	else if(print_type == PRINT_INFO){

		__mpcprintf(messagebuffer, modulename, filename, line, color);
	}
	va_end (ap);
	return r;
}