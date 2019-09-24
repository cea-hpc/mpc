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

#ifndef MPC_COMMON_INCLUDE_MPC_COMMON_IO_HELPER_H_
#define MPC_COMMON_INCLUDE_MPC_COMMON_IO_HELPER_H_

#include <unistd.h>

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

/***********************
 * COMMON IO FUNCTIONS *
 ***********************/

/**
 * @brief Parse a long from a string with error handling
 *
 * @param input string containing a value
 * @return long long value as parsed (abort on error)
 */
long mpc_common_parse_long(char * input);

/*!
 * Call read in loop to avoid problem with splitted messages.
 * Also support EINTR error if interrupted.
 * @param fd File descriptor to read.
 * @param buf Buffer to read.
 * @param count Size of buffer to read.
*/
ssize_t mpc_common_io_safe_read(int fd,void * buf,size_t count);

/*!
 * Call write in loop to avoid problem with splitted messages.
 * Also support EINTR error if interrupted.
 * @param fd File descriptor to write.
 * @param buf Buffer to write.
 * @param count Size of buffer to write.
*/
ssize_t mpc_common_io_safe_write(int fd,const void * buf,size_t count);

#endif /* MPC_COMMON_INCLUDE_MPC_COMMON_IO_HELPER_H_ */
