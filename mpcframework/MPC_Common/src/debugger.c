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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_config.h>
#include <sctk_debug.h>
#include <mpc_common_debugger.h>

#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <debugger.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_helper.h>

#ifdef MPC_HAVE_LIBUNWIND
	#define UNW_LOCAL_ONLY
	#include <libunwind.h>
#endif

#ifdef HAVE_EXECINFO_H
	#include <execinfo.h>
#endif

static void determine_elf_class( char *name, elf_class_t *c )
{
	Elf32_Ehdr header;
	c->name = name;
	c->fd = fopen( c->name, "r" );
	sctk_nodebug( "determine_elf_class %s", c->name );

	if ( c->fd != NULL )
	{
		fread( &header, sizeof( Elf32_Ehdr ), 1, c->fd );

		if ( header.e_ident[EI_CLASS] == ELFCLASS32 )
		{
			c->read_elf_header = sctk_read_elf_header_32;
			c->read_elf_sections = sctk_read_elf_section_32;
			c->read_elf_symbols = sctk_read_elf_symbols_32;
			c->read_elf_sym = sctk_read_elf_sym_32;
		}
		else if ( header.e_ident[EI_CLASS] == ELFCLASS64 )
		{
			c->read_elf_header = sctk_read_elf_header_64;
			c->read_elf_sections = sctk_read_elf_section_64;
			c->read_elf_symbols = sctk_read_elf_symbols_64;
			c->read_elf_sym = sctk_read_elf_sym_64;
		}
		else
		{
			fclose( c->fd );
			c->fd = NULL;
		}

		if ( c->fd )
		{
			rewind( c->fd );
		}
	}
	else
	{
		c->read_elf_header = NULL;
		c->read_elf_sections = NULL;
		c->read_elf_symbols = NULL;
		c->read_elf_sym = NULL;
	}

	c->sections = NULL;
	c->string_table = NULL;
	c->sym_table = NULL;
	c->dynsym_table = NULL;
	c->symbols = NULL;
	c->debug_line = NULL;
	c->debug_str = NULL;
	c->debug_aranges = NULL;
	c->file_list = NULL;
	c->dir_list = NULL;
	c->is_lib = 0;
	c->dbg_list = NULL;
	c->dbg_nb = 0;
	c->debug_info = NULL;
}

#if 0
static void
release_elf ( elf_class_t *c )
{
	sctk_free ( c->sections );
	sctk_free ( c->string_table );
	sctk_free ( c->sym_table );
	sctk_free ( c->dynsym_table );
	sctk_free ( c->symbols );
	sctk_free ( c->debug_line );
	sctk_free ( c->debug_aranges );
	sctk_free ( c->file_list );
	sctk_free ( c->dir_list );
	sctk_free ( c->dbg_list );
	sctk_free ( c->debug_str );
}
#endif

#if defined( DEBUG )
static void
print_map()
{
	int pid;
	static char name[4096];
	static char buffer[4096 * 1024];
	unsigned long readed = 0;
	FILE *in;
	pid = getpid();
	sprintf( name, "/proc/%d/maps", pid );
	in = fopen( name, "r" );

	do
	{
		readed = fread( buffer, 1, 4096 * 1024, in );
		fwrite( buffer, 1, readed, stdout );
	}
	while ( readed != 0 );

	fclose( in );
}
#endif

typedef struct
{
	char file[4096];
	char *start;
	char *end;
	elf_class_t c;
} map_line_t;

static map_line_t *map = NULL;
static int nb_lines = 0;

static void read_map()
{
	int pid;
	static char name[4096];
	static char buffer[4096 * 1024];
	static int done = 0;
	FILE *in;
	char *line;
	char *ptr;
	int i;

	if ( done == 1 )
	{
		return;
	}

	pid = getpid();
	sprintf( name, "/proc/%d/maps", pid );
	in = fopen( name, "r" );

	do
	{
		line = fgets( buffer, 4096 * 1024, in );

		if ( line != NULL )
		{
			uintptr_t tmp_ptr;
			long tmp_ptr_l;
			nb_lines++;
			map =
			    ( map_line_t * ) realloc( map,
			                              ( nb_lines + 1 ) * sizeof( map_line_t ) );
			map[nb_lines - 1].start = NULL;
			map[nb_lines - 1].end = NULL;
			memset( map[nb_lines - 1].file, '\0', 4096 );
			map[nb_lines - 1].start = ( void * ) ( 0 );
			map[nb_lines - 1].end = ( void * ) ( -1 );
			memset( map[nb_lines].file, '\0', 4096 );
			ptr = buffer;
			/*       fprintf(stderr,"%s",ptr);  */
			tmp_ptr = strtoll( ptr, &ptr, 16 );
			tmp_ptr_l = ( long ) tmp_ptr;
			map[nb_lines - 1].start = ( char * ) ( tmp_ptr_l );
			ptr++;
			tmp_ptr = strtoll( ptr, &ptr, 16 );
			tmp_ptr_l = ( long ) tmp_ptr;
			map[nb_lines - 1].end = ( char * ) ( tmp_ptr_l );

			while ( ( *ptr != '/' ) && ( *ptr != '\n' ) )
			{
				ptr++;
			}

			memcpy( map[nb_lines - 1].file, ptr, strlen( ptr ) - 1 );
		}
	}
	while ( line != NULL );

	fclose( in );

	/*   fprintf(stderr,"%d lines\n",nb_lines); */
	for ( i = 0; i < nb_lines; i++ )
	{
		/*     fprintf(stderr,"%010p %010p %s\n",map[i].start,map[i].end,map[i].file); */
		determine_elf_class( map[i].file, &map[i].c );

		if ( map[i].c.read_elf_header != NULL )
		{
			map[i].c.read_elf_header( &map[i].c );
			map[i].c.read_elf_sections( &map[i].c );
			map[i].c.read_elf_symbols( &map[i].c );
		}
	}

	done = 1;
}

static mpc_common_spinlock_t big_lock = SCTK_SPINLOCK_INITIALIZER;

void sctk_vprint_backtrace( const char *format, va_list ap )
{
	mpc_common_spinlock_lock( &big_lock );
	mpc_common_io_noalloc_fprintf( stderr, "---------- EVENT TRACE START ----------\n" );
	mpc_common_io_noalloc_vfprintf( stderr, format, ap );
#ifdef MPC_HAVE_LIBUNWIND
	unw_cursor_t cursor;
	unw_context_t uc;
	unw_word_t ip, sp;
	unw_word_t offset;
	char func_name_buf[10000];
	int i;
	read_map();
	unw_getcontext( &uc );
	unw_init_local( &cursor, &uc );
	int frame_id = 0;
	char *previous_lib = NULL;
	int first_lib = 1;

	while ( unw_step( &cursor ) > 0 )
	{
		mpc_addr2line_t ptr;
		memset( &ptr, 0, sizeof( mpc_addr2line_t ) );
		unw_get_reg( &cursor, UNW_REG_IP, &ip );
		unw_get_reg( &cursor, UNW_REG_SP, &sp );
		unw_get_proc_name( &cursor, func_name_buf, 10000, &offset );
		ptr.ptr = ( void * ) ip;

		for ( i = 0; i < nb_lines; i++ )
		{
			if ( ( ( unsigned long ) ip >= ( unsigned long ) map[i].start ) &&
			     ( ( unsigned long ) ip < ( unsigned long ) map[i].end ) )
			{
				break;
			}
		}

		sctk_nodebug( "ip %p", ptr.ptr );

		if ( map[i].c.is_lib )
		{
			ptr.ptr =
			    ( char * ) ( ( unsigned long ) ptr.ptr - ( unsigned long ) map[i].start );
		}

		sctk_nodebug( "ip %p in lib %d %d", ptr.ptr, i, nb_lines );

		if ( map[i].c.read_elf_sym != NULL )
		{
			map[i].c.read_elf_sym( &map[i].c, &ptr, 1 );
		}
		else
		{
			ptr.line = -1;
		}

		sctk_nodebug( "ip %p in lib %d %d read map done line %d", ptr.ptr, i,
		              nb_lines, ptr.line );

		if ( strcmp( func_name_buf, "" ) == 0 )
		{
			sprintf( func_name_buf, ptr.name );
		}


		if ( !previous_lib || ( strcmp( previous_lib, map[i].file ) ) )
		{
			if(first_lib == 0)
			{
				mpc_common_io_noalloc_fprintf( stderr, "\n"SCTK_COLOR_VIOLET_BOLD(})"\n" );
			}
			first_lib=0;
			mpc_common_io_noalloc_fprintf( stderr, SCTK_COLOR_VIOLET_BOLD( %s )"\n"SCTK_COLOR_VIOLET_BOLD({)"\n", map[i].file );
						previous_lib = map[i].file;
		}


		sctk_nodebug( "ip %p", ip );
		sctk_nodebug( "%-50s", func_name_buf );
		sctk_nodebug( "abs %p", ptr.absolute );
		sctk_nodebug( "dir %p", ptr.dir );
		sctk_nodebug( "file %p", ptr.file );
		sctk_nodebug( "dir %s", ptr.dir );
		sctk_nodebug( "file %s", ptr.file );
		sctk_nodebug( "absolute %s", ptr.absolute );
		mpc_common_io_noalloc_fprintf( stderr, "\t[%2d] "SCTK_COLOR_RED_BOLD( %s )" @ "SCTK_COLOR_YELLOW( %llx )"\n", frame_id, func_name_buf, ip );

		if ( ptr.line != ( unsigned long ) - 1 )
		{
			mpc_common_io_noalloc_fprintf( stderr, "\t     "SCTK_COLOR_BLUE_BOLD( %s/%s/%s:%d )"\n", ptr.absolute,
							ptr.dir, ptr.file, ptr.line );
		}

		frame_id++;
	}

	mpc_common_io_noalloc_fprintf( stderr, "\n"SCTK_COLOR_VIOLET_BOLD(})"\n" );

#elif defined HAVE_EXECINFO_H
	void *array[20];
	size_t size;
	mpc_common_spinlock_lock( &big_lock );
	read_map();
	size = 20;
#if 0
	{
		int i;
		backtrace_symbols ( array, size );

		for ( i = 0; i < size; i++ )
		{
			mpc_addr2line_t ptr;
			int j;
			char func_name_buf[10000];
			void *ip;
			sctk_nodebug ( "%p", array[i] );
			memset ( &ptr, 0, sizeof ( mpc_addr2line_t ) );
			ip = array[i];
			ptr.ptr = ip;

			for ( j = 0; j < nb_lines; j++ )
			{
				if ( ( ( unsigned long ) ip >= ( unsigned long ) map[j].start ) &&
				     ( ( unsigned long ) ip < ( unsigned long ) map[j].end ) )
				{
					break;
				}
			}

			if ( map[j].c.is_lib )
			{
				ptr.ptr =
				    ( char * ) ( ( unsigned long ) ptr.ptr -
				                 ( unsigned long ) map[j].start );
			}

			if ( map[j].c.read_elf_sym != NULL )
			{
				map[j].c.read_elf_sym ( &map[j].c, &ptr, 1 );
			}
			else
			{
				ptr.line = -1;
			}

			sprintf ( func_name_buf, ptr.name );

			if ( ptr.line != -1 )
			{
				mpc_common_io_noalloc_fprintf ( stderr, "%16llx %-50s %s/%s/%s:%d (%s)\n",
				                                ( uintptr_t ) ip,
				                                /*ptr.name, */ func_name_buf,
				                                ptr.absolute,
				                                ptr.dir, ptr.file, ptr.line, map[i].file );
			}
			else
			{
				mpc_common_io_noalloc_fprintf ( stderr, "%16llx %-50s (%s)\n",
				                                ( uintptr_t ) ip,
				                                /*ptr.name, */ func_name_buf,
				                                map[i].file );
			}

			if ( strcmp ( func_name_buf, "sctk_thread_create_tmp_start_routine" ) ==
			     0 )
			{
				break;
			}
		}
	}
#endif
	backtrace_symbols_fd( array, size, 2 );
#else
	void *array[20];
	size_t size;
	int i;
	mpc_common_spinlock_lock( &big_lock );
	read_map();
	mpc_common_io_noalloc_fprintf( stderr, "---------- EVENT TRACE START ----------\n" );
	mpc_common_io_noalloc_vfprintf( stderr, format, ap );
	mpc_common_io_noalloc_fprintf( stderr, "UNABLE TO BACKTRACE\n" );
#endif
mpc_common_io_noalloc_fprintf( stderr, "----------- EVENT TRACE END -----------\n" );
mpc_common_spinlock_unlock( &big_lock );
}

void mpc_common_debuger_print_backtrace( const char *format, ... )
{
	va_list ap;
	va_start( ap, format );
	sctk_vprint_backtrace( format, ap );
	va_end( ap );
}

void mpc_common_debuger_resolve( mpc_addr2line_t *tab, int size )
{
	int j;
	int i;
	mpc_common_spinlock_lock( &big_lock );
	read_map();

	for ( j = 0; j < size; j++ )
	{
		mpc_addr2line_t ptr;
		ptr.ptr = tab[j].ptr;

		for ( i = 0; i < nb_lines; i++ )
		{
			if ( ( ( unsigned long ) tab[j].ptr >=
			       ( unsigned long ) map[i].start ) &&
			     ( ( unsigned long ) tab[j].ptr < ( unsigned long ) map[i].end ) )
			{
				break;
			}
		}

		if ( map[i].c.is_lib )
		{
			ptr.ptr =
			    ( char * ) ( ( unsigned long ) ptr.ptr - ( unsigned long ) map[i].start );
		}

		if ( map[i].c.read_elf_sym != NULL )
		{
			map[i].c.read_elf_sym( &map[i].c, &ptr, 1 );
		}
		else
		{
			ptr.line = -1;
		}

		ptr.ptr = tab[j].ptr;
		memcpy( &( tab[j] ), &ptr, sizeof( mpc_addr2line_t ) );
	}

	mpc_common_spinlock_unlock( &big_lock );
}

/**********************************************************************/
/* Backtrace on Signal                                                */
/**********************************************************************/

void mpc_common_debugger_sig_handler( int sig, siginfo_t *info, __UNUSED__ void *arg )
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
	mpc_common_debuger_print_backtrace( "" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "*************************************************\n" );
	fprintf( stderr, "* MPC has tried to backtrace but note that      *\n" );
	fprintf( stderr, "* this operation is not signal safe you may     *\n" );
	fprintf( stderr, "* get either an incomplete or invalid backtrace *\n" );
	fprintf( stderr, "* you may now launch GDB -- good debugging      *\n" );
	fprintf( stderr, "*************************************************\n" );
	fprintf( stderr, "\n" );
	sctk_error(
	    "/!\\ CHECK THE ACTUAL SIGNAL IN PREVIOUS ERROR (may not be SIGSEV)" );
	MPC_CRASH();
}

void mpc_common_debugger_install_sig_handlers()
{
	char *cmd = getenv( "MPC_BT_SIG" );
	struct sigaction action;
	memset( &action, 0, sizeof( action ) );
	action.sa_sigaction = mpc_common_debugger_sig_handler;
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