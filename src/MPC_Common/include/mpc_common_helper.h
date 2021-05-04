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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <mpc_config.h>

/***********************
 * SHELL COLOR SUPPORT *
 ***********************/

#ifdef MPC_ENABLE_SHELL_COLORS

	#define MPC_COLOR_ESC "\033["
	/* Normal colors */
	#define MPC_COLOR_RED(txt)             MPC_COLOR_ESC"31m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_GREEN(txt)           MPC_COLOR_ESC"32m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_YELLOW(txt)          MPC_COLOR_ESC"33m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_BLUE(txt)            MPC_COLOR_ESC"34m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_VIOLET(txt)          MPC_COLOR_ESC"35m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_GRAY(txt)            MPC_COLOR_ESC"30m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_WHITE(txt)           MPC_COLOR_ESC"37m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_CYAN(txt)            MPC_COLOR_ESC"36m"#txt MPC_COLOR_ESC"0m"
	/* Bold colors */
	#define MPC_COLOR_RED_BOLD(txt)        MPC_COLOR_ESC"1;31m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_GREEN_BOLD(txt)      MPC_COLOR_ESC"1;32m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_YELLOW_BOLD(txt)     MPC_COLOR_ESC"1;33m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_BLUE_BOLD(txt)       MPC_COLOR_ESC"1;34m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_VIOLET_BOLD(txt)     MPC_COLOR_ESC"1;35m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_GRAY_BOLD(txt)       MPC_COLOR_ESC"1;30m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_WHITE_BOLD(txt)      MPC_COLOR_ESC"1;37m"#txt MPC_COLOR_ESC"0m"
	#define MPC_COLOR_CYAN_BOLD(txt)       MPC_COLOR_ESC"1;36m"#txt MPC_COLOR_ESC"0m"

#else

	/* Normal colors */
	#define MPC_COLOR_RED(txt)             #txt
	#define MPC_COLOR_GREEN(txt)           #txt
	#define MPC_COLOR_YELLOW(txt)          #txt
	#define MPC_COLOR_BLUE(txt)            #txt
	#define MPC_COLOR_VIOLET(txt)          #txt
	#define MPC_COLOR_GRAY(txt)            #txt
	#define MPC_COLOR_WHITE(txt)           #txt
	#define MPC_COLOR_CYAN(txt)            #txt
	/* Bold colors */
	#define MPC_COLOR_RED_BOLD(txt)        #txt
	#define MPC_COLOR_GREEN_BOLD(txt)      #txt
	#define MPC_COLOR_YELLOW_BOLD(txt)     #txt
	#define MPC_COLOR_BLUE_BOLD(txt)       #txt
	#define MPC_COLOR_VIOLET_BOLD(txt)     #txt
	#define MPC_COLOR_GRAY_BOLD(txt)       #txt
	#define MPC_COLOR_WHITE_BOLD(txt)      #txt
	#define MPC_COLOR_CYAN_BOLD(txt)       #txt

#endif /* SHELL_COLORS */

/***********
 * DEFINES *
 ***********/

#define MPC_COMMON_MAX_STRING_SIZE  512
#define MPC_COMMON_MAX_FILENAME_SIZE 1024

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

/***********
 * SIGNALS *
 ***********/

void mpc_common_helper_ignore_sigpipe();

/****************
 * MEMORY STATS *
 ****************/

size_t mpc_common_helper_memory_in_use( void );

/*****************
 * CAST INTEGERS *
 *****************/

#ifdef MPC_64_BIT_ARCH
static inline int sctk_safe_cast_long_int(long l)
{
#ifdef SCTK_VERIFY_CAST
	assume(INT_MIN <= l);
	assume(l <= INT_MAX);
#endif
	return ( int )l;
}

#else
#define sctk_safe_cast_long_int(l)    l
#endif


/**********************
 * NETWORKING HELPERS *
 **********************/

/**
 * @brief Returns true if a given address is part of a given network
 *
 * @param network_addr network device address
 * @param network_mask network mask
 * @param candidate candidate address
 * @return int 1 if device is part of the net supported by this device
 */
int mpc_common_address_in_range(in_addr_t network_addr, in_addr_t network_mask, in_addr_t candidate);


/**
 * @brief Get the network interface name supporting a given IPV4 address
 *
 * @param addr address to check for support
 * @param ifname output interface name
 * @param ifname_len length of output buffer
 * @return int 0 if resolution succeeded
 */
int mpc_common_getaddr_interface(struct sockaddr_in *addr, char * ifname, int ifname_len);

/**
 * @brief Get the network interface supporting a given socket
 *
 * @param socket socket to look for
 * @param ifname output interface name
 * @param ifname_len length of output buffer
 * @return int 0 if resolution succeeded
 */
int mpc_common_getsocket_interface(int socket, char * ifname, int ifname_len);

/**
 * @brief This is a wrapper aroung getaddrinfo which reorders results with preferred interface
 *
 * @param node see getaddrinfo
 * @param service see getaddrinfo
 * @param hints see getaddrinfo
 * @param res see getaddrinfo (NOTE !! free it with mpc_common_freeaddrinfo)
 * @param preferred_device Either the exact device name or part of it (nothing done if "")
 * @return int see getaddrinfo
 */
int mpc_common_getaddrinfo(const char *node, const char *service,
                           const struct addrinfo *hints,
                           struct addrinfo **res,
                           const char *preferred_device);

/**
 * @brief Free a list as returned by mpc_common_getaddrinfo
 *
 * @param res the response to free
 */
void mpc_common_freeaddrinfo(struct addrinfo *res);

#endif /* MPC_COMMON_INCLUDE_MPC_COMMON_HELPER_H_ */
