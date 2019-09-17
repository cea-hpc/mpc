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

/* Escape char */
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

#endif
