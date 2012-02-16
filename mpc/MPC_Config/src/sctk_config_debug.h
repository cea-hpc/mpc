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
/* #   - VALAT Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_DEBUG_H
#define SCTK_DEBUG_H

/********************  HEADERS  *********************/
#include <stdlib.h>
#include <stdio.h>

/************************** MACROS *************************/
/** Print some warning into error output, it use the printf standard parameters. **/
#define warning(...) {fprintf(stderr,"Warning at %s!%d\n",__FILE__,__LINE__); fprintf(stderr,__VA_ARGS__);}
/** Assert a value, and display a user readable message if invalide. It will be keep with NDEBUG on the contrary of assert(). **/
#define assume(x,...) if (!(x)) { fprintf(stderr,"Error at %s!%d\n%s\n",__FILE__,__LINE__,#x); fprintf(stderr,__VA_ARGS__); abort(); }
/** Print an error message and exit. It use the print formatting convention. **/
#define fatal(...) { fprintf(stderr,"Fatal error at %s!%d\n",__FILE__,__LINE__); fprintf(stderr,__VA_ARGS__); abort(); }

#endif
