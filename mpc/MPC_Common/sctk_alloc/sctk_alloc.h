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
#ifndef __SCTK__ALLOC__INT__
#define __SCTK__ALLOC__INT__

#include "sctk_config.h"
#include <sys/mman.h>

//TODO Remove this for version >2.4.1 if no issue were found
#if 0
#if defined(SCTK_USE_VALGRIND)

#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>

#if !defined(VALGRIND_STACK_REGISTER) || !defined(VALGRIND_STACK_REGISTER) || !defined(VALGRIND_MAKE_NOACCESS)|| !defined(VALGRIND_MAKE_WRITABLE)|| !defined(VALGRIND_MALLOCLIKE_BLOCK)|| !defined(VALGRIND_FREELIKE_BLOCK) || !defined(VALGRIND_MAKE_READABLE)
#error "Unable to compil with Valgrind support"
#endif

#define SCTK_MAKE_NOACCESS(tmp,size) VALGRIND_MAKE_NOACCESS(tmp,size)
#define SCTK_MAKE_WRITABLE(a,b) VALGRIND_MAKE_WRITABLE(a,b)
#define SCTK_MAKE_READABLE(a,b) VALGRIND_MAKE_READABLE(a,b)
#define SCTK_CHECK_READABLE(a,b) VALGRIND_CHECK_READABLE(a,b)

#define SCTK_MALLOCLIKE_BLOCK(a,b,c,d) VALGRIND_MALLOCLIKE_BLOCK(a,b,c,d)
#define SCTK_FREELIKE_BLOCK(a,b) VALGRIND_FREELIKE_BLOCK(a,b)

#else

#define SCTK_MAKE_NOACCESS(tmp,size) (void)(0)
#define SCTK_MAKE_WRITABLE(a,b) (void)(0)
#define SCTK_MAKE_READABLE(a,b) (void)(0)
#define SCTK_CHECK_READABLE(a,b) (void)(0)

#define SCTK_MALLOCLIKE_BLOCK(a,b,c,d) (void)(0)
#define SCTK_FREELIKE_BLOCK(a,b) (void)(0)
#endif
#endif

#ifdef HAVE_MEMCHECK_H
	#include <valgrind/valgrind.h>
	#include <valgrind/memcheck.h>
#else //HAVE_MEMCHECK_H
	#define VALGRIND_STACK_REGISTER(a,b) (void)(0)
#endif


#ifndef MPC_PosixAllocator
#include "sctk_no_alloc.h"
#else
#include <sctk_alloc_posix.h>
#include <sctk_alloc_on_node.h>
#endif

#endif
