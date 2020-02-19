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

#define HAS_UCTX

#ifdef HAS_UCTX
	#include <sys/ucontext.h>
#endif

#include "mpc_common_rank.h"
#include "sctk_debug.h"
#include "mpc_common_spinlock.h"

#include "mpc_common_helper.h"


#ifdef MPC_Debugger
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
/*Backtrace                                                           */
/**********************************************************************/

void sctk_c_backtrace( char *reason, void *override )
{
	fprintf( stderr, "============== BACKTRACE =============== [%s]\n", reason );
	void *bts[100];
	int bs = backtrace( bts, 100 );

	if ( override )
	{
		bts[1] = override;
	}

	backtrace_symbols_fd( bts + 2, bs - 2, STDERR_FILENO );
	fprintf( stderr, "========================================\n" );
}

void sctk_debug_print_backtrace( const char *format, ... )
{
	va_list ap;
	va_start( ap, format );
#ifdef MPC_Debugger
	sctk_vprint_backtrace( format, ap );
#else
	char reason[100];
	vsnprintf( reason, 100, format, ap );
	sctk_c_backtrace( reason, NULL );
#endif
	va_end( ap );
}

/**********************************************************************/
/*Abort                                                               */
/**********************************************************************/
#ifdef MPC_Launch
	void mpc_launch_pmi_abort();
#endif
void sctk_abort( void )
{
	static volatile int done = 0;
	sctk_debug_print_backtrace( "sctk_abort" );

#ifdef MPC_Launch
	if ( done == 0 )
	{
		done = 1;
		mpc_launch_pmi_abort();
	}
#endif
	abort();
}

/**********************************************************************/
/* Backtrace on Signal                                                */
/**********************************************************************/

void backtrace_sig_handler( int sig, siginfo_t *info, void *arg )
{
	char *ssig = strsignal( sig );

	if ( sig == SIGINT )
	{
		fprintf( stderr, "MPC has been interrupted by user --> SIGINT\n" );
		exit( sig );
	}

	sctk_error( "==========================================================" );
	sctk_error( "                                                          " );
	sctk_error( "Process Caught signal %s(%d)", ssig, sig );
	sctk_error( "                                                          " );

	switch ( sig )
	{
		case SIGILL:
		case SIGFPE:
		case SIGTRAP:
			sctk_error( "Instruction : %p ", info->si_addr );
			break;

		case SIGBUS:
		case SIGSEGV:
			sctk_error( "Faulty address was : %p ", info->si_addr );
			break;
	}

	sctk_error( "                                                          " );
	sctk_error( " INFO : Disable signal capture by exporting MPC_BT_SIG=0  " );
	sctk_error( "        or through MPC's configuration file (mpc_bt_sig)  " );
	sctk_error( "                                                          " );
	sctk_error( "!!! MPC will now segfault indiferently from the signal !!!" );
	sctk_error( "==========================================================" );
#ifdef HAS_UCTX
	fprintf( stderr, "\n" );
	ucontext_t *context = ( ucontext_t * ) arg;
	void *caller_add = NULL;

	if ( context )
	{
#if defined( __i386__ )
		caller_add = ( void * ) context->uc_mcontext.gregs[REG_EIP];
#elif defined( __x86_64__ )
		caller_add = ( void * ) context->uc_mcontext.gregs[REG_RIP];
#endif
	}

	sctk_c_backtrace( ssig, caller_add );
	fprintf( stderr, "\n" );
	fprintf( stderr, "*************************************************\n" );
	fprintf( stderr, "* MPC has tried to backtrace but note that      *\n" );
	fprintf( stderr, "* this operation is not signal safe you may     *\n" );
	fprintf( stderr, "* get either an incomplete or invalid backtrace *\n" );
	fprintf( stderr, "* you may now launch GDB -- good debugging      *\n" );
	fprintf( stderr, "*************************************************\n" );
	fprintf( stderr, "\n" );
#endif /* HAS_UCTX */
	sctk_error(
	    "/!\\ CHECK THE ACTUAL SIGNAL IN PREVIOUS ERROR (may not be SIGSEV)" );
	CRASH();
}

void sctk_install_bt_sig_handler()
{
	char *cmd = getenv( "MPC_BT_SIG" );
	struct sigaction action;
	memset( &action, 0, sizeof( action ) );
	action.sa_sigaction = backtrace_sig_handler;
	action.sa_flags = SA_SIGINFO;

	if ( cmd )
	{
		/* Forced disable */
		if ( !strcmp( cmd, "0" ) )
		{
			return;
		}
	}

        sigaction( SIGSEGV, &action, NULL );
        sigaction( SIGHUP, &action, NULL );
        sigaction( SIGINT, &action, NULL );
        sigaction( SIGILL, &action, NULL );
        sigaction( SIGFPE, &action, NULL );
        sigaction( SIGQUIT, &action, NULL );
        sigaction( SIGBUS, &action, NULL );
        sigaction( SIGTERM, &action, NULL );


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
        char info_message[SMALL_BUFFER_SIZE];
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

/**********************************************************************/
/*Other                                                               */
/**********************************************************************/

void sctk_init( void )
{

}

void sctk_leave( void )
{
}
