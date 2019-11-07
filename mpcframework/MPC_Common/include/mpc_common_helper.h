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
/* #   - VALAT Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPC_COMMON_INCLUDE_MPC_COMMON_HELPER_H_
#define MPC_COMMON_INCLUDE_MPC_COMMON_HELPER_H_

#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

/***********************
 * SHELL COLOR SUPPORT *
 ***********************/

#ifdef SHELL_COLORS

	#define SCTK_COLOR_ESC "\033["
	/* Normal colors */
	#define SCTK_COLOR_RED(txt)             SCTK_COLOR_ESC"31m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_GREEN(txt)           SCTK_COLOR_ESC"32m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_YELLOW(txt)          SCTK_COLOR_ESC"33m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_BLUE(txt)            SCTK_COLOR_ESC"34m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_VIOLET(txt)          SCTK_COLOR_ESC"35m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_GRAY(txt)            SCTK_COLOR_ESC"30m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_WHITE(txt)           SCTK_COLOR_ESC"37m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_CYAN(txt)            SCTK_COLOR_ESC"36m"#txt SCTK_COLOR_ESC"0m"
	/* Bold colors */
	#define SCTK_COLOR_RED_BOLD(txt)        SCTK_COLOR_ESC"1;31m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_GREEN_BOLD(txt)      SCTK_COLOR_ESC"1;32m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_YELLOW_BOLD(txt)     SCTK_COLOR_ESC"1;33m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_BLUE_BOLD(txt)       SCTK_COLOR_ESC"1;34m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_VIOLET_BOLD(txt)     SCTK_COLOR_ESC"1;35m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_GRAY_BOLD(txt)       SCTK_COLOR_ESC"1;30m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_WHITE_BOLD(txt)      SCTK_COLOR_ESC"1;37m"#txt SCTK_COLOR_ESC"0m"
	#define SCTK_COLOR_CYAN_BOLD(txt)       SCTK_COLOR_ESC"1;36m"#txt SCTK_COLOR_ESC"0m"

#else

	/* Normal colors */
	#define SCTK_COLOR_RED(txt)             #txt
	#define SCTK_COLOR_GREEN(txt)           #txt
	#define SCTK_COLOR_YELLOW(txt)          #txt
	#define SCTK_COLOR_BLUE(txt)            #txt
	#define SCTK_COLOR_VIOLET(txt)          #txt
	#define SCTK_COLOR_GRAY(txt)            #txt
	#define SCTK_COLOR_WHITE(txt)           #txt
	#define SCTK_COLOR_CYAN(txt)            #txt
	/* Bold colors */
	#define SCTK_COLOR_RED_BOLD(txt)        #txt
	#define SCTK_COLOR_GREEN_BOLD(txt)      #txt
	#define SCTK_COLOR_YELLOW_BOLD(txt)     #txt
	#define SCTK_COLOR_BLUE_BOLD(txt)       #txt
	#define SCTK_COLOR_VIOLET_BOLD(txt)     #txt
	#define SCTK_COLOR_GRAY_BOLD(txt)       #txt
	#define SCTK_COLOR_WHITE_BOLD(txt)      #txt
	#define SCTK_COLOR_CYAN_BOLD(txt)       #txt

#endif /* SHELL_COLORS */

/***********
 * DEFINES *
 ***********/

#define MPC_COMMON_MAX_STRING_SIZE  512

/*************
 * MIN & MAX *
 *************/

/**
 * @brief Return the maximum of two values
 *
 */
#define mpc_common_max( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )

/**
 * @brief Return the minimum of two values
 *
 */
#define mpc_common_min( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )

/*********************
 * HASHING FUNCTIONS *
 *********************/

static inline uint64_t mpc_common_hash( uint64_t val )
{
	/* This is MURMUR Hash under MIT
	 * https://code.google.com/p/smhasher/ */
	uint64_t h = val;
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdllu;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53llu;
	h ^= h >> 33;
	return h;
}

/***********************
 * COMMON IO FUNCTIONS *
 ***********************/

/**
 * @brief Parse a long from a string with error handling
 *
 * @param input string containing a value
 * @return long long value as parsed (abort on error)
 */
long mpc_common_parse_long( char *input );

/*!
 * Call read in loop to avoid problem with splitted messages.
 * Also support EINTR error if interrupted.
 * @param fd File descriptor to read.
 * @param buf Buffer to read.
 * @param count Size of buffer to read.
*/
ssize_t mpc_common_io_safe_read( int fd, void *buf, size_t count );

/*!
 * Call write in loop to avoid problem with splitted messages.
 * Also support EINTR error if interrupted.
 * @param fd File descriptor to write.
 * @param buf Buffer to write.
 * @param count Size of buffer to write.
*/
ssize_t mpc_common_io_safe_write( int fd, const void *buf, size_t count );


/*********************
 * COMMON NOALLOC IO *
 *********************/

#define MPC_COMMON_MAX_NOALLOC_IO_SIZE (4*1024)

int mpc_common_io_noalloc_vsnprintf ( char *s, size_t n, const char *format, va_list ap );
int mpc_common_io_noalloc_snprintf ( char *restrict s, size_t n, const char *restrict format, ... );
size_t mpc_common_io_noalloc_fwrite ( const void *ptr, size_t size, size_t nmemb, FILE *stream );
int mpc_common_io_noalloc_fprintf ( FILE *stream, const char *format, ... );
int mpc_common_io_noalloc_vfprintf ( FILE *stream, const char *format, va_list ap );
int mpc_common_io_noalloc_printf ( const char *format, ... );

/*************************
 * MPC INTERNAL PROFILER *
 *************************/

#ifdef MPC_Profiler

	#include "sctk_internal_profiler.h"

#else

	#define SCTK_PROFIL_START(key)	(void)(0)
	#define SCTK_PROFIL_END(key) (void)(0)
	#define SCTK_COUNTER_INC(key,val) (void)(0);
	#define SCTK_COUNTER_DEC(key,val) (void)(0);
	#define SCTK_PROFIL_END_WITH_VALUE(key, value) (void)(0);

	#define sctk_internal_profiler_init() (void)(0)
	#define sctk_internal_profiler_render() (void)(0)
	#define sctk_internal_profiler_release() (void)(0)

#endif /* MPC_Profiler */

#endif /* MPC_COMMON_INCLUDE_MPC_COMMON_HELPER_H_ */
